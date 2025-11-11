#include "divisors.hpp"

#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    long long n;
    std::cout << "Enter an integer: ";
    if (!(std::cin >> n)) {
        std::cerr << "Invalid input." << '\n';
        return 1;
    }

    if (n == 0) {
        std::cout << "Zero has infinitely many divisors." << '\n';
        return 0;
    }

    std::vector<long long> divisors = calculateDivisors(n);

    std::cout << "Divisors: ";
    for (std::size_t idx = 0; idx < divisors.size(); ++idx) {
        if (idx > 0) {
            std::cout << ", ";
        }
        std::cout << divisors[idx];
    }
    std::cout << '\n';

    return 0;
}
