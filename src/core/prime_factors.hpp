#pragma once

#include <vector>
#include <utility>

// Returns the prime factors of n as pairs of (prime, exponent).
// Throws std::invalid_argument if n is less than or equal to zero.
std::vector<std::pair<long long, int>> calculatePrimeFactors(long long n);
