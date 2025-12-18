#include "variables.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

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

std::string toLower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}
} // namespace

VariableStore::VariableStore(std::string filePath)
    : filePath_(std::move(filePath)) {}

bool VariableStore::load() {
  vars_.clear();
  std::ifstream file(filePath_);
  if (!file) {
    return true; // Missing file means no variables yet.
  }

  std::string line;
  while (std::getline(file, line)) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;
    }
    if (trimmed.front() == '[' && trimmed.back() == ']') {
      continue;
    }

    auto equals = trimmed.find('=');
    if (equals == std::string::npos) {
      continue;
    }

    std::string name = trim(trimmed.substr(0, equals));
    std::string valuePart = trim(trimmed.substr(equals + 1));
    if (!isValidName(name)) {
      continue;
    }

    try {
      double value = std::stod(valuePart);
      vars_[normalizeName(name)] = value;
    } catch (const std::exception &) {
      continue;
    }
  }

  return true;
}

bool VariableStore::save() const {
  std::ofstream file(filePath_, std::ios::trunc);
  if (!file) {
    return false;
  }

  file << "# CLI Calculator variables\n";
  file << "[variables]\n";
  file << std::setprecision(std::numeric_limits<double>::digits10 + 1);
  for (const auto &entry : vars_) {
    file << entry.first << " = " << entry.second << '\n';
  }

  return true;
}

const std::map<std::string, double> &VariableStore::variables() const {
  return vars_;
}

std::optional<double> VariableStore::find(const std::string &name) const {
  auto normalized = normalizeName(name);
  auto it = vars_.find(normalized);
  if (it == vars_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void VariableStore::set(const std::string &name, double value) {
  vars_[normalizeName(name)] = value;
}

bool VariableStore::remove(const std::string &name) {
  auto normalized = normalizeName(name);
  auto it = vars_.find(normalized);
  if (it == vars_.end()) {
    return false;
  }
  vars_.erase(it);
  return true;
}

bool VariableStore::isValidName(const std::string &name) {
  if (name.empty()) {
    return false;
  }
  std::string trimmed = trim(name);
  if (trimmed.empty()) {
    return false;
  }
  if (!std::isalpha(static_cast<unsigned char>(trimmed.front()))) {
    return false;
  }
  for (char ch : trimmed) {
    if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_') {
      return false;
    }
  }
  return true;
}

std::string VariableStore::normalizeName(const std::string &name) {
  return toLower(trim(name));
}

VariableStore &globalVariableStore() {
  static VariableStore store("vars.toml");
  return store;
}
