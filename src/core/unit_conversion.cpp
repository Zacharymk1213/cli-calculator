#include "unit_conversion.hpp"

namespace {
const std::vector<LinearCategory> kLinearCategories = {
    {"Length",
     {{"Meter", "m", 1.0},
      {"Kilometer", "km", 1000.0},
      {"Centimeter", "cm", 0.01},
      {"Millimeter", "mm", 0.001},
      {"Mile", "mi", 1609.344},
      {"Yard", "yd", 0.9144},
      {"Foot", "ft", 0.3048},
      {"Inch", "in", 0.0254}}},
    {"Mass",
     {{"Kilogram", "kg", 1.0},
      {"Gram", "g", 0.001},
      {"Milligram", "mg", 0.000001},
      {"Metric ton", "t", 1000.0},
      {"Pound", "lb", 0.45359237},
      {"Ounce", "oz", 0.028349523125}}},
    {"Volume",
     {{"Liter", "L", 1.0},
      {"Milliliter", "mL", 0.001},
      {"Cubic meter", "m^3", 1000.0},
      {"Gallon (US)", "gal", 3.78541},
      {"Pint (US)", "pt", 0.473176}}}};

const std::vector<TemperatureUnit> kTemperatureUnits = {
    {"Celsius", "C", TemperatureScale::Celsius},
    {"Fahrenheit", "F", TemperatureScale::Fahrenheit},
    {"Kelvin", "K", TemperatureScale::Kelvin}};

double toCelsius(double value, TemperatureScale scale) {
  switch (scale) {
  case TemperatureScale::Celsius:
    return value;
  case TemperatureScale::Fahrenheit:
    return (value - 32.0) * 5.0 / 9.0;
  case TemperatureScale::Kelvin:
    return value - 273.15;
  }
  return value;
}

double fromCelsius(double value, TemperatureScale scale) {
  switch (scale) {
  case TemperatureScale::Celsius:
    return value;
  case TemperatureScale::Fahrenheit:
    return (value * 9.0 / 5.0) + 32.0;
  case TemperatureScale::Kelvin:
    return value + 273.15;
  }
  return value;
}
} // namespace

const std::vector<LinearCategory> &linearCategories() {
  return kLinearCategories;
}

const std::vector<TemperatureUnit> &temperatureUnits() {
  return kTemperatureUnits;
}

double convertLinearValue(double value, const LinearUnit &from,
                          const LinearUnit &to) {
  double baseValue = value * from.toBaseFactor;
  return baseValue / to.toBaseFactor;
}

double convertTemperature(double value, const TemperatureUnit &from,
                          const TemperatureUnit &to) {
  double celsius = toCelsius(value, from.scale);
  return fromCelsius(celsius, to.scale);
}
