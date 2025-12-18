#include "divisors.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

std::vector<long long> calculateDivisors(long long n) {
  if (n == 0) {
    throw std::invalid_argument("Zero has infinitely many divisors.");
  }

  long long absN = n < 0 ? -n : n;
  std::vector<long long> divisors;

  for (long long i = 1; i * i <= absN; ++i) {
    if (absN % i == 0) {
      divisors.push_back(i);
      if (i != absN / i) {
        divisors.push_back(absN / i);
      }
    }
  }

  std::sort(divisors.begin(), divisors.end());
  return divisors;
}
