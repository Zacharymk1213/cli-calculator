#pragma once

#include <vector>

// Returns the positive divisors of the absolute value of n in ascending order.
// Throws std::invalid_argument if n is zero.
std::vector<long long> calculateDivisors(long long n);
