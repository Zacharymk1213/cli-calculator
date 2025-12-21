#include "parse_utils.hpp"

#include <algorithm>
#include <sstream>
#include <string>

std::vector<std::string> parseCsvLine(const std::string &line) {
  std::vector<std::string> fields;
  std::string field;
  bool inQuotes = false;
  for (std::size_t idx = 0; idx < line.size(); ++idx) {
    char ch = line[idx];
    if (ch == '"') {
      if (inQuotes && idx + 1 < line.size() && line[idx + 1] == '"') {
        field.push_back('"');
        ++idx;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (ch == ',' && !inQuotes) {
      fields.push_back(field);
      field.clear();
    } else {
      field.push_back(ch);
    }
  }
  fields.push_back(field);
  return fields;
}

bool parseNumberList(const std::string &input, std::vector<double> &values,
                     std::string &error) {
  values.clear();
  std::string sanitized = input;
  std::replace(sanitized.begin(), sanitized.end(), ',', ' ');
  std::istringstream stream(sanitized);
  std::string token;
  while (stream >> token) {
    try {
      std::size_t processed = 0;
      double value = std::stod(token, &processed);
      if (processed != token.size()) {
        error = "Invalid number: " + token;
        return false;
      }
      values.push_back(value);
    } catch (const std::exception &) {
      error = "Invalid number: " + token;
      return false;
    }
  }
  if (values.empty()) {
    error = "Please enter at least one numeric value.";
    return false;
  }
  return true;
}
