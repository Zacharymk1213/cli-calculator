#include "prime_factors.hpp"

#include <stdexcept>

std::vector<std::pair<long long, int>> calculatePrimeFactors(long long n)
{
    if (n <= 0)
    {
        throw std::invalid_argument("Prime factorization defined only for positive integers.");
    }

    std::vector<std::pair<long long, int>> factors;
    long long remaining = n;

    for (long long divisor = 2; divisor <= remaining / divisor; ++divisor)
    {
        int exponent = 0;
        while (remaining % divisor == 0)
        {
            ++exponent;
            remaining /= divisor;
        }
        if (exponent > 0)
        {
            factors.emplace_back(divisor, exponent);
        }
    }

    if (remaining > 1)
    {
        factors.emplace_back(remaining, 1);
    }

    return factors;
}
