#pragma once

#include <string>
#include <vector>

struct LinearUnit {
  std::string name;
  std::string symbol;
  double toBaseFactor;
};

struct LinearCategory {
  std::string name;
  std::vector<LinearUnit> units;
};

enum class TemperatureScale { Celsius, Fahrenheit, Kelvin };

struct TemperatureUnit {
  std::string name;
  std::string symbol;
  TemperatureScale scale;
};

const std::vector<LinearCategory> &linearCategories();
const std::vector<TemperatureUnit> &temperatureUnits();

double convertLinearValue(double value, const LinearUnit &from,
                          const LinearUnit &to);
double convertTemperature(double value, const TemperatureUnit &from,
                          const TemperatureUnit &to);
