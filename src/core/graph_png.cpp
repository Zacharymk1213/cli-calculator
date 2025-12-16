#include "graph_png.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <zlib.h>

namespace {
struct ImageBuffer {
  std::size_t width;
  std::size_t height;
  std::vector<std::uint8_t> pixels;

  ImageBuffer(std::size_t w, std::size_t h)
      : width(w), height(h), pixels(w * h * 4, 255) {}
};

void setPixel(ImageBuffer &image, int x, int y,
              const std::array<std::uint8_t, 4> &color) {
  if (x < 0 || y < 0) {
    return;
  }
  if (static_cast<std::size_t>(x) >= image.width ||
      static_cast<std::size_t>(y) >= image.height) {
    return;
  }
  std::size_t index =
      (static_cast<std::size_t>(y) * image.width +
       static_cast<std::size_t>(x)) *
      4;
  for (std::size_t channel = 0; channel < 4; ++channel) {
    image.pixels[index + channel] = color[channel];
  }
}

void drawLine(ImageBuffer &image, int x0, int y0, int x1, int y1,
              const std::array<std::uint8_t, 4> &color) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int steps = std::max(std::abs(dx), std::abs(dy));
  if (steps == 0) {
    setPixel(image, x0, y0, color);
    return;
  }

  for (int step = 0; step <= steps; ++step) {
    double progress = static_cast<double>(step) / static_cast<double>(steps);
    int x = static_cast<int>(std::lround(
        static_cast<double>(x0) + progress * static_cast<double>(dx)));
    int y = static_cast<int>(std::lround(
        static_cast<double>(y0) + progress * static_cast<double>(dy)));
    setPixel(image, x, y, color);
  }
}

void drawPoint(ImageBuffer &image, int x, int y,
               const std::array<std::uint8_t, 4> &color) {
  constexpr int radius = 2;
  for (int offsetY = -radius; offsetY <= radius; ++offsetY) {
    for (int offsetX = -radius; offsetX <= radius; ++offsetX) {
      if (offsetX * offsetX + offsetY * offsetY <= radius * radius) {
        setPixel(image, x + offsetX, y + offsetY, color);
      }
    }
  }
}

constexpr int FONT_HEIGHT = 7;
constexpr int SPACE_WIDTH = 3;
constexpr int FONT_SPACING = 1;

using Glyph = std::array<std::string, FONT_HEIGHT>;

const std::unordered_map<char, Glyph> kFont = {
    {'0',
     {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'1',
     {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'2',
     {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {'3',
     {" ### ", "#   #", "    #", " ### ", "    #", "#   #", " ### "}},
    {'4',
     {"#   #", "#   #", "#   #", "#####", "    #", "    #", "    #"}},
    {'5',
     {"#####", "#    ", "#    ", "#### ", "    #", "#   #", " ### "}},
    {'6',
     {" ### ", "#   #", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {'7',
     {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {'8',
     {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {'9',
     {" ### ", "#   #", "#   #", " ####", "    #", "#   #", " ### "}},
    {'-',
     {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
    {'.',
     {"     ", "     ", "     ", "     ", "     ", "  ## ", "  ## "}},
};

int glyphWidth(char ch) {
  if (ch == ' ') {
    return SPACE_WIDTH;
  }
  auto it = kFont.find(ch);
  if (it == kFont.end()) {
    return SPACE_WIDTH;
  }
  return static_cast<int>(it->second.front().size());
}

int measureTextWidth(const std::string &text) {
  if (text.empty()) {
    return 0;
  }
  int width = 0;
  for (std::size_t idx = 0; idx < text.size(); ++idx) {
    width += glyphWidth(text[idx]);
    if (idx + 1 < text.size()) {
      width += FONT_SPACING;
    }
  }
  return width;
}

void drawGlyph(ImageBuffer &image, int x, int y, char ch,
               const std::array<std::uint8_t, 4> &color) {
  auto it = kFont.find(ch);
  if (it == kFont.end()) {
    return;
  }
  const auto &rows = it->second;
  for (int row = 0; row < FONT_HEIGHT; ++row) {
    const std::string &pattern = rows[row];
    for (int column = 0; column < static_cast<int>(pattern.size()); ++column) {
      if (pattern[column] != ' ') {
        setPixel(image, x + column, y + row, color);
      }
    }
  }
}

void drawText(ImageBuffer &image, int x, int y, const std::string &text,
              const std::array<std::uint8_t, 4> &color) {
  int penX = x;
  for (std::size_t idx = 0; idx < text.size(); ++idx) {
    char ch = text[idx];
    if (ch != ' ') {
      drawGlyph(image, penX, y, ch, color);
    }
    penX += glyphWidth(ch);
    if (idx + 1 < text.size()) {
      penX += FONT_SPACING;
    }
  }
}

std::string trimTrailingZeros(std::string value) {
  if (value.find('.') == std::string::npos) {
    if (value == "-0") {
      return "0";
    }
    return value;
  }
  while (!value.empty() && value.back() == '0') {
    value.pop_back();
  }
  if (!value.empty() && value.back() == '.') {
    value.pop_back();
  }
  if (value.empty() || value == "-0") {
    return "0";
  }
  return value;
}

std::string formatAxisLabel(double value, double fullRange) {
  double magnitude = std::fabs(fullRange);
  int precision = 0;
  if (magnitude < 1.0) {
    precision = 3;
  } else if (magnitude < 10.0) {
    precision = 2;
  } else if (magnitude < 100.0) {
    precision = 1;
  } else {
    precision = 0;
  }
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return trimTrailingZeros(stream.str());
}

void writeUint32(std::vector<std::uint8_t> &buffer, std::uint32_t value) {
  buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
  buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
  buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
  buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

void encodeUint32(std::vector<std::uint8_t> &buffer, std::size_t offset,
                  std::uint32_t value) {
  buffer[offset + 0] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
  buffer[offset + 1] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
  buffer[offset + 2] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
  buffer[offset + 3] = static_cast<std::uint8_t>(value & 0xFF);
}

bool writePng(const std::string &path, std::size_t width, std::size_t height,
              const std::vector<std::uint8_t> &pixels, std::string &error) {
  if (pixels.size() != width * height * 4) {
    error = "Unexpected image buffer length.";
    return false;
  }

  const std::size_t stride = width * 4;
  std::vector<std::uint8_t> raw((stride + 1) * height);
  for (std::size_t y = 0; y < height; ++y) {
    std::size_t rowStart = y * (stride + 1);
    raw[rowStart] = 0; // filter method None
    std::memcpy(raw.data() + rowStart + 1, pixels.data() + y * stride, stride);
  }

  uLongf compressedSize = compressBound(raw.size());
  std::vector<std::uint8_t> compressed(compressedSize);
  int status = compress2(compressed.data(), &compressedSize, raw.data(),
                         raw.size(), Z_BEST_COMPRESSION);
  if (status != Z_OK) {
    error = "Unable to compress PNG payload.";
    return false;
  }
  compressed.resize(compressedSize);

  std::vector<std::uint8_t> png;
  png.reserve(8 + 25 + compressed.size());
  const unsigned char signature[8] = {0x89, 'P', 'N', 'G',
                                      0x0D, 0x0A, 0x1A, 0x0A};
  png.insert(png.end(), std::begin(signature), std::end(signature));

  auto appendChunk = [&](const char type[4],
                         const std::vector<std::uint8_t> &data) {
    writeUint32(png, static_cast<std::uint32_t>(data.size()));
    std::size_t typeOffset = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, png.data() + typeOffset,
                static_cast<std::uint32_t>(4 + data.size()));
    writeUint32(png, static_cast<std::uint32_t>(crc));
  };

  std::vector<std::uint8_t> ihdr(13, 0);
  encodeUint32(ihdr, 0, static_cast<std::uint32_t>(width));
  encodeUint32(ihdr, 4, static_cast<std::uint32_t>(height));
  ihdr[8] = 8;  // bit depth
  ihdr[9] = 6;  // color type RGBA
  ihdr[10] = 0; // compression method
  ihdr[11] = 0; // filter method
  ihdr[12] = 0; // no interlace
  appendChunk("IHDR", ihdr);

  appendChunk("IDAT", compressed);
  appendChunk("IEND", {});

  std::ofstream output(path, std::ios::binary);
  if (!output) {
    error = "Unable to open output file '" + path + "'.";
    return false;
  }
  output.write(reinterpret_cast<const char *>(png.data()),
               static_cast<std::streamsize>(png.size()));
  if (!output) {
    error = "Failed to write PNG image to disk.";
    return false;
  }
  return true;
}
} // namespace

bool generateGraphPng(const std::vector<double> &values,
                      const std::string &outputPath, std::string &error) {
  if (values.empty()) {
    error = "No data to plot.";
    return false;
  }

  std::size_t width = std::max<std::size_t>(600, values.size() * 40);
  std::size_t height = 400;
  ImageBuffer image(width, height);

  const std::array<std::uint8_t, 4> axisColor = {64, 64, 64, 255};
  const std::array<std::uint8_t, 4> gridColor = {220, 220, 220, 255};
  const std::array<std::uint8_t, 4> lineColor = {31, 119, 180, 255};
  const std::array<std::uint8_t, 4> pointColor = {214, 39, 40, 255};

  const int leftMargin = 60;
  const int rightMargin = 30;
  const int topMargin = 30;
  const int bottomMargin = 50;
  const int plotWidth =
      static_cast<int>(width) - leftMargin - rightMargin;
  const int plotHeight =
      static_cast<int>(height) - topMargin - bottomMargin;
  if (plotWidth <= 0 || plotHeight <= 0) {
    error = "Image dimensions are too small for plotting.";
    return false;
  }

  double minValue =
      *std::min_element(values.begin(), values.end());
  double maxValue =
      *std::max_element(values.begin(), values.end());
  double valueRange = maxValue - minValue;
  double normalizedRange = valueRange;
  if (normalizedRange == 0.0) {
    normalizedRange = 1.0;
  }

  // Draw grid lines for readability.
  constexpr int horizontalGridLines = 4;
  std::vector<int> horizontalTickYs;
  std::vector<double> horizontalTickValues;
  horizontalTickYs.reserve(horizontalGridLines + 1);
  horizontalTickValues.reserve(horizontalGridLines + 1);
  for (int row = 0; row <= horizontalGridLines; ++row) {
    double ratio =
        static_cast<double>(row) / static_cast<double>(horizontalGridLines);
    int y = topMargin +
            static_cast<int>(std::lround(ratio * static_cast<double>(plotHeight)));
    drawLine(image, leftMargin, y, leftMargin + plotWidth, y, gridColor);
    horizontalTickYs.push_back(y);
    double tickValue = maxValue - ratio * valueRange;
    horizontalTickValues.push_back(tickValue);
  }
  constexpr int verticalGridLines = 6;
  for (int column = 0; column <= verticalGridLines; ++column) {
    double ratio = static_cast<double>(column) /
                   static_cast<double>(verticalGridLines);
    int x = leftMargin +
            static_cast<int>(std::lround(ratio * static_cast<double>(plotWidth)));
    drawLine(image, x, topMargin, x, topMargin + plotHeight, gridColor);
  }

  // Draw axes on top of the grid.
  drawLine(image, leftMargin, topMargin, leftMargin,
           topMargin + plotHeight, axisColor);
  drawLine(image, leftMargin, topMargin + plotHeight,
           leftMargin + plotWidth, topMargin + plotHeight, axisColor);

  std::vector<std::pair<int, int>> points;
  points.reserve(values.size());
  for (std::size_t idx = 0; idx < values.size(); ++idx) {
    double normalized = (values[idx] - minValue) / normalizedRange;
    if (normalized < 0.0) {
      normalized = 0.0;
    }
    if (normalized > 1.0) {
      normalized = 1.0;
    }
    int x = leftMargin;
    if (values.size() > 1) {
      double ratio = static_cast<double>(idx) /
                     static_cast<double>(values.size() - 1);
      x += static_cast<int>(
          std::lround(ratio * static_cast<double>(plotWidth)));
    }
    int y = topMargin + plotHeight -
            static_cast<int>(
                std::lround(normalized * static_cast<double>(plotHeight)));
    points.emplace_back(x, y);
  }

  if (points.size() == 1) {
    drawPoint(image, points.front().first, points.front().second, pointColor);
  } else {
    for (std::size_t idx = 1; idx < points.size(); ++idx) {
      drawLine(image, points[idx - 1].first, points[idx - 1].second,
               points[idx].first, points[idx].second, lineColor);
    }
    for (const auto &point : points) {
      drawPoint(image, point.first, point.second, pointColor);
    }
  }

  const std::array<std::uint8_t, 4> textColor = {20, 20, 20, 255};
  const int tickLength = 6;
  const int labelPadding = 4;
  const int axisY = topMargin + plotHeight;

  for (std::size_t idx = 0; idx < horizontalTickYs.size(); ++idx) {
    int y = horizontalTickYs[idx];
    drawLine(image, leftMargin - tickLength, y, leftMargin, y, axisColor);
    std::string label =
        formatAxisLabel(horizontalTickValues[idx], valueRange);
    int textWidth = measureTextWidth(label);
    int textX = leftMargin - tickLength - labelPadding - textWidth;
    if (textX < 0) {
      textX = 0;
    }
    int textY = y - FONT_HEIGHT / 2;
    if (textY < 0) {
      textY = 0;
    }
    drawText(image, textX, textY, label, textColor);
  }

  std::vector<std::size_t> xTickIndices;
  const std::size_t maxTicks = 8;
  if (values.size() <= maxTicks) {
    for (std::size_t idx = 0; idx < values.size(); ++idx) {
      xTickIndices.push_back(idx);
    }
  } else {
    const std::size_t desired = maxTicks;
    std::vector<std::size_t> raw;
    raw.reserve(desired);
    for (std::size_t idx = 0; idx < desired; ++idx) {
      double ratio =
          static_cast<double>(idx) / static_cast<double>(desired - 1);
      std::size_t position = static_cast<std::size_t>(std::lround(
          ratio * static_cast<double>(values.size() - 1)));
      raw.push_back(position);
    }
    std::sort(raw.begin(), raw.end());
    raw.erase(std::unique(raw.begin(), raw.end()), raw.end());
    xTickIndices = std::move(raw);
  }
  if (xTickIndices.empty()) {
    xTickIndices.push_back(0);
  }
  if (xTickIndices.front() != 0) {
    xTickIndices.insert(xTickIndices.begin(), 0);
  }
  if (xTickIndices.back() != values.size() - 1) {
    xTickIndices.push_back(values.size() - 1);
  }
  xTickIndices.erase(std::unique(xTickIndices.begin(), xTickIndices.end()),
                     xTickIndices.end());

  for (std::size_t index : xTickIndices) {
    int x = points[index].first;
    drawLine(image, x, axisY, x, axisY + tickLength, axisColor);
    std::string label = std::to_string(index + 1);
    int textWidth = measureTextWidth(label);
    int textX = x - textWidth / 2;
    if (textX < 0) {
      textX = 0;
    }
    int maxTextX = static_cast<int>(width) - textWidth;
    if (textX > maxTextX) {
      textX = maxTextX;
    }
    int textY = axisY + tickLength + labelPadding;
    if (textY + FONT_HEIGHT > static_cast<int>(height)) {
      textY = static_cast<int>(height) - FONT_HEIGHT;
    }
    drawText(image, textX, textY, label, textColor);
  }

  return writePng(outputPath, width, height, image.pixels, error);
}
