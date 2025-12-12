#include "math_utils.hpp"

bool isApproximatelyZero(double value, double epsilon) {
  return std::abs(value) <= epsilon;
}
