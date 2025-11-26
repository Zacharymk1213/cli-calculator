#include "cli_actions.hpp"

#include "ansi_colors.hpp"
#include "divisors.hpp"
#include "expression.hpp"
#include "numeral_conversion.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

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
                double result = evaluateExpression(expression);
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
                return 1;
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
                return 1;
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
                    return 1;
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
    }

    return std::nullopt;
}
