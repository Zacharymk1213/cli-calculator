#include "cli_actions.hpp"
#include "ansi_colors.hpp"
#include "core/variables.hpp"
#include "divisors.hpp"
#include "expression.hpp"
#include "numeral_conversion.hpp"
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <prime_factors.hpp>
#include <sstream>


std::optional<int> handleCommandLine(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--no-color" || arg == "-nc")
        {
            continue;
        }

        if (arg == "--eval" || arg == "-e")
        {
            if (i + 1 >= argc)
            {
                std::cerr << RED << "Error: missing expression after " << arg << RESET << '\n';
                return 1;
            }
            try
            {
                std::string expression(argv[i + 1]);
                double result = evaluateExpression(expression, globalVariableStore().variables());
                std::cout << GREEN << "Result: " << RESET << result << '\n';
                return 0;
            }
            catch (const std::exception &ex)
            {
                std::cout << RED << "Error: " << RESET << ex.what() << '\n';
                return 1;
            }
        }

        if (arg == "--square-root" || arg == "-sqrt")
        {
            if (i + 1 >= argc)
            {
                std::cerr << RED << "Error: missing value after " << arg << RESET << '\n';
                return 1;
            }
            try
            {
                std::string number(argv[i + 1]);
                double value = std::stod(number);
                if (value < 0.0)
                {
                    std::cerr << RED << "Error: square root undefined for negative values." << RESET << '\n';
                    return 1;
                }
                double result = std::sqrt(value);
                std::cout << GREEN << "Result: " << RESET << result << '\n';
                return 0;
            }
            catch (const std::exception &ex)
            {
                std::cerr << RED << "Error: unable to parse number: " << ex.what() << RESET << '\n';
                return 1;
            }
        }

        if (arg == "--divisors" || arg == "-d")
        {
            if (i + 1 >= argc)
            {
                std::cerr << RED << "Error: Missing value after " << arg << RESET << '\n';
                return 2;
            }
            try
            {
                std::string input(argv[i + 1]);
                long long n = std::stoll(input);
                std::vector<long long> result = calculateDivisors(n);
                std::cout << GREEN << "Divisors: " << RESET;
                for (std::size_t idx = 0; idx < result.size(); ++idx)
                {
                    if (idx > 0)
                    {
                        std::cout << ", ";
                    }
                    std::cout << result[idx];
                }
                std::cout << '\n';
                return 0;
            }
            catch (const std::exception &ex)
            {
                std::cerr << RED << "Error: unable to parse number: " << ex.what() << RESET << '\n';
                return 1;
            }
        }

        if (arg == "--convert" || arg == "-c")
        {
            if (i + 3 >= argc)
            {
                std::cerr << RED << "Error: missing arguments after " << arg << RESET << '\n';
                return 2;
            }
            try
            {
                std::string fromBaseStr(argv[i + 1]);
                std::string toBaseStr(argv[i + 2]);
                std::string valueStr(argv[i + 3]);

                int fromBase = std::stoi(fromBaseStr);
                int toBase = std::stoi(toBaseStr);
                if ((fromBase != 2 && fromBase != 10 && fromBase != 16) ||
                    (toBase != 2 && toBase != 10 && toBase != 16))
                {
                    std::cerr << RED << "Error: bases must be 2, 10, or 16." << RESET << '\n';
                    return 2;
                }
                long long decimalValue = parseInteger(valueStr, fromBase);
                std::string converted = formatInteger(decimalValue, toBase);
                std::cout << GREEN << "Result: " << RESET << converted << '\n';
                return 0;
            }
            catch (const std::exception &ex)
            {
                std::cerr << RED << "Error: unable to perform conversion: " << ex.what() << RESET << '\n';
                return 1;
            }
        }
        if (arg == "--help" || arg == "-h")
        {
            std::cout << BOLD << BLUE << "CLI Calculator Help" << RESET << '\n';
            std::cout << "Usage: calculator [options]\n";
            std::cout << "Options:\n";
            std::cout << "  -e, --eval <expression>       Evaluate the given mathematical expression.\n";
            std::cout << "  -sqrt, --square-root <value>  Calculate the square root of the given value.\n";
            std::cout << "  -d, --divisors <number>       Calculate and display the divisors of the given number.\n";
            std::cout << "  -c, --convert <from> <to> <value>  Convert value from one base to another (bases: 2, 10, 16).\n";
            std::cout << "  -nc, --no-color               Disable colored output.\n";
            std::cout << "  -h, --help                    Display this help message.\n";
            return 0;
        }
        if (arg == "--prime-factorization" || arg == "-pf")
        {
            if (i + 1 >= argc)
                {
                    std::cerr << RED << "Error: missing arguments after " << arg << RESET << '\n';
                    return 2;
                }
            try
            {
                std::string input(argv[i + 1]);
                long long value = std::stoll(input);
                if (value == 0 || value == 1 || value == -1)
                {
                    std::cout << YELLOW << value << " has no prime factors." << RESET << '\n';
                    return 0;
                } else
                {
                    long long absValue = value < 0 ? -value : value;
                try
                {
                    auto factors = calculatePrimeFactors(absValue);
                    std::vector<std::string> parts;
                    if (value < 0)
                    {
                        parts.push_back("-1");
                    }
                    for (const auto &factor : factors)
                    {
                        std::ostringstream part;
                        part << factor.first;
                        if (factor.second > 1)
                        {
                            part << '^' << factor.second;
                        }
                        parts.push_back(part.str());
                    }
                    std::cout << GREEN << "Prime factorization: " << RESET;
                    for (std::size_t idx = 0; idx < parts.size(); ++idx)
                    {
                        if (idx > 0)
                        {
                            std::cout << " * ";
                        }
                        std::cout << parts[idx];
                    }
                    std::cout << '\n';
                }
                catch (const std::exception &ex)
                {
                    std::cout << RED << "Error: " << RESET << ex.what() << '\n';
                }
                }
                return 0;
            }
            catch (const std::exception &ex)
            {
                std::cerr << RED << "Error: unable to parse number: " << ex.what() << RESET << '\n';
                return 1;
            }
        }
    }

    return std::nullopt;
}
