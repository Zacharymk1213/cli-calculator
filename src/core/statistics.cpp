#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::vector<double> sortValues(const std::vector<double> &values) {
  std::vector<double> sorted(values);
  std::sort(sorted.begin(), sorted.end());
  return sorted;
}
} // namespace

StatisticsSummary calculateStatistics(const std::vector<double> &values) {
  if (values.empty()) {
    throw std::invalid_argument("statistics require at least one value");
  }

  StatisticsSummary summary;
  summary.count = values.size();
  summary.sum = std::accumulate(values.begin(), values.end(), 0.0);
  summary.mean = summary.sum / static_cast<double>(summary.count);

  std::vector<double> sorted = sortValues(values);
  summary.minimum = sorted.front();
  summary.maximum = sorted.back();
  summary.range = summary.maximum - summary.minimum;

  if (summary.count % 2 == 0) {
    std::size_t rightIndex = summary.count / 2;
    summary.median = (sorted[rightIndex - 1] + sorted[rightIndex]) / 2.0;
  } else {
    summary.median = sorted[summary.count / 2];
  }

  double variance = 0.0;
  for (double value : values) {
    double diff = value - summary.mean;
    variance += diff * diff;
  }
  summary.variance = variance / static_cast<double>(summary.count);
  summary.standardDeviation = std::sqrt(summary.variance);

  // Track modes; only keep them when they are meaningful (frequency > 1).
  std::size_t maxFrequency = 0;
  std::size_t currentCount = 0;
  double currentValue = sorted.front();

  auto flushCurrent = [&](double value) {
    if (currentCount > maxFrequency) {
      maxFrequency = currentCount;
      summary.modes.clear();
      summary.modes.push_back(value);
    } else if (currentCount == maxFrequency) {
      summary.modes.push_back(value);
    }
  };

  for (double value : sorted) {
    if (value == currentValue) {
      ++currentCount;
    } else {
      flushCurrent(currentValue);
      currentValue = value;
      currentCount = 1;
    }
  }
  flushCurrent(currentValue);

  if (maxFrequency <= 1) {
    summary.modes.clear();
  }
  return summary;
}

double calculatePercentile(const std::vector<double> &values,
                           double percentile) {
  if (values.empty()) {
    throw std::invalid_argument("percentile requires at least one value");
  }
  if (percentile < 0.0 || percentile > 100.0) {
    throw std::invalid_argument("percentile must be between 0 and 100");
  }

  std::vector<double> sorted = sortValues(values);
  if (sorted.size() == 1) {
    return sorted.front();
  }

  double scaled = percentile / 100.0 * static_cast<double>(sorted.size() - 1);
  std::size_t lowerIndex = static_cast<std::size_t>(std::floor(scaled));
  std::size_t upperIndex = static_cast<std::size_t>(std::ceil(scaled));
  double fraction = scaled - static_cast<double>(lowerIndex);
  double lower = sorted[lowerIndex];
  double upper = sorted[upperIndex];
  return lower + (upper - lower) * fraction;
}

std::vector<std::string> buildAsciiGraph(const std::vector<double> &values,
                                         std::size_t height) {
  if (values.empty()) {
    return {"(no data to graph)"};
  }
  if (height < 2) {
    height = 2;
  }

  double minValue = *std::min_element(values.begin(), values.end());
  double maxValue = *std::max_element(values.begin(), values.end());
  double range = maxValue - minValue;
  if (range == 0.0) {
    range = 1.0;
  }

  std::vector<std::size_t> filledHeight;
  filledHeight.reserve(values.size());
  if (height == 1) {
    filledHeight.assign(values.size(), 0);
  } else {
    for (double value : values) {
      double ratio = (value - minValue) / range;
      std::size_t amount =
          static_cast<std::size_t>(std::round(ratio * (height - 1)));
      filledHeight.push_back(amount);
    }
  }

  std::vector<std::string> lines;
  lines.reserve(height + 2);
  int precision = 2;

  for (std::size_t row = 0; row < height; ++row) {
    double levelValue = maxValue - (static_cast<double>(row) /
                                    static_cast<double>(height - 1)) *
                                       (maxValue - minValue);
    std::ostringstream prefix;
    prefix << std::fixed << std::setprecision(precision) << std::setw(8)
           << levelValue << " | ";
    std::string line = prefix.str();
    std::size_t bottomIndex = height - row - 1;
    for (std::size_t column = 0; column < values.size(); ++column) {
      if (filledHeight[column] >= bottomIndex) {
        line.push_back('#');
      } else {
        line.push_back(' ');
      }
    }
    lines.push_back(line);
  }

  std::ostringstream axis;
  axis << std::string(11, ' ') << '+' << std::string(values.size(), '-');
  lines.push_back(axis.str());

  std::ostringstream legend;
  legend << "Count: " << values.size() << "  Min: " << std::fixed
         << std::setprecision(2) << minValue << "  Max: " << maxValue;
  lines.push_back(legend.str());

  return lines;
}
