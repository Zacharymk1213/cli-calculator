#include "numeral_conversion.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace {
std::string trim(const std::string &text) {
  auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
  auto begin = std::find_if(text.begin(), text.end(), notSpace);
  if (begin == text.end()) {
    return "";
  }
  auto rbegin = std::find_if(text.rbegin(), text.rend(), notSpace);
  return {begin, rbegin.base()};
}

std::string normalizeForBase(const std::string &input, int base) {
  std::string trimmed = trim(input);
  if (trimmed.empty()) {
    throw std::invalid_argument("Empty input.");
  }

  std::string sign;
  std::size_t pos = 0;
  if (trimmed[0] == '+' || trimmed[0] == '-') {
    sign = trimmed.substr(0, 1);
    pos = 1;
  }

  std::string body = trimmed.substr(pos);
  auto hasPrefix = [](const std::string &value, char expected) {
    if (value.size() <= 1 || value[0] != '0') {
      return false;
    }
    char prefixChar =
        static_cast<char>(std::tolower(static_cast<unsigned char>(value[1])));
    return prefixChar == expected;
  };

  if (base == 2 && hasPrefix(body, 'b')) {
    body = body.substr(2);
  } else if (base == 16 && hasPrefix(body, 'x')) {
    body = body.substr(2);
  }

  if (body.empty()) {
    throw std::invalid_argument("No digits were provided.");
  }

  return sign + body;
}
} // namespace

long long parseInteger(const std::string &text, int base) {
  std::string normalized = normalizeForBase(text, base);
  std::size_t processed = 0;
  long long value = 0;
  try {
    value = std::stoll(normalized, &processed, base);
  } catch (const std::invalid_argument &) {
    throw std::invalid_argument("Invalid character among the digits.");
  } catch (const std::out_of_range &) {
    throw std::out_of_range("The provided number is too large to convert.");
  }

  if (processed != normalized.size()) {
    throw std::invalid_argument("Invalid character among the digits.");
  }

  return value;
}

std::string formatInteger(long long value, int base) {
  if (base == 10) {
    return std::to_string(value);
  }

  const char *digits = "0123456789ABCDEF";
  bool negative = value < 0;
  unsigned long long magnitude =
      negative ? static_cast<unsigned long long>(-(value + 1)) + 1ULL
               : static_cast<unsigned long long>(value);

  if (magnitude == 0) {
    return "0";
  }

  std::string converted;
  while (magnitude > 0) {
    converted.push_back(digits[magnitude % base]);
    magnitude /= base;
  }

  std::reverse(converted.begin(), converted.end());

  std::string prefix;
  if (base == 2) {
    prefix = "0b";
  } else if (base == 16) {
    prefix = "0x";
  }

  return (negative ? "-" : "") + prefix + converted;
}
