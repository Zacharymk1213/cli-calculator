#include "cli_commands.hpp"

#include "ansi_colors.hpp"
#include "cli_numeric.hpp"
#include "core/variables.hpp"
#include "divisors.hpp"
#include "expression.hpp"
#include "numeral_conversion.hpp"
#include "prime_factors.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

int runEval(const std::string &expression, OutputFormat outputFormat, std::optional<double> *lastResult)
{
    if (lastResult)
    {
        lastResult->reset();
    }
    try
    {
        double result = evaluateExpression(expression, globalVariableStore().variables());
        if (outputFormat == OutputFormat::Json)
        {
            std::ostringstream payload;
            payload << "\"expression\":\"" << jsonEscape(expression) << "\",\"result\":" << result;
            printJsonSuccess(std::cout, "eval", payload.str());
        }
        else
        {
            std::cout << GREEN << "Result: " << RESET << result << '\n';
        }
        if (lastResult)
        {
            *lastResult = result;
        }
        return 0;
    }
    catch (const std::exception &ex)
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cout, "eval", ex.what());
        }
        else
        {
            std::cout << RED << "Error: " << RESET << ex.what() << '\n';
        }
        if (lastResult)
        {
            lastResult->reset();
        }
        return 1;
    }
}

int runSquareRoot(const std::string &number, OutputFormat outputFormat, std::optional<double> *lastResult)
{
    if (lastResult)
    {
        lastResult->reset();
    }
    double value = 0.0;
    std::string parseError;
    if (!resolveDoubleArgument(number, value, parseError))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "square-root", parseError);
        }
        else
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        return 1;
    }
    if (value < 0.0)
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "square-root", "square root undefined for negative values.");
        }
        else
        {
            std::cerr << RED << "Error: square root undefined for negative values." << RESET << '\n';
        }
        return 1;
    }
    double result = std::sqrt(value);
    if (outputFormat == OutputFormat::Json)
    {
        std::ostringstream payload;
        payload << "\"value\":" << value << ",\"result\":" << result;
        printJsonSuccess(std::cout, "square-root", payload.str());
    }
    else
    {
        std::cout << GREEN << "Result: " << RESET << result << '\n';
    }
    if (lastResult)
    {
        *lastResult = result;
    }
    return 0;
}

int runDivisors(const std::string &input, OutputFormat outputFormat)
{
    long long n = 0;
    std::string parseError;
    if (!resolveIntegerArgument(input, n, parseError))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "divisors", parseError);
        }
        else
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        return 1;
    }

    std::vector<long long> result = calculateDivisors(n);
    if (outputFormat == OutputFormat::Json)
    {
        std::ostringstream payload;
        payload << "\"number\":" << n << ",\"divisors\":[";
        for (std::size_t idx = 0; idx < result.size(); ++idx)
        {
            if (idx > 0)
            {
                payload << ',';
            }
            payload << result[idx];
        }
        payload << "]";
        printJsonSuccess(std::cout, "divisors", payload.str());
    }
    else
    {
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
    }
    return 0;
}

int runConvert(const std::string &fromBaseStr, const std::string &toBaseStr, const std::string &valueStr, OutputFormat outputFormat)
{
    long long fromBaseValue = 0;
    long long toBaseValue = 0;
    std::string parseError;
    if (!resolveIntegerArgument(fromBaseStr, fromBaseValue, parseError))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "convert", parseError);
        }
        else
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        return 1;
    }
    if (!resolveIntegerArgument(toBaseStr, toBaseValue, parseError))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "convert", parseError);
        }
        else
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        return 1;
    }
    if (fromBaseValue < std::numeric_limits<int>::min() || fromBaseValue > std::numeric_limits<int>::max() ||
        toBaseValue < std::numeric_limits<int>::min() || toBaseValue > std::numeric_limits<int>::max())
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "convert", "base is out of supported range.");
        }
        else
        {
            std::cerr << RED << "Error: base is out of supported range." << RESET << '\n';
        }
        return 1;
    }
    int fromBase = static_cast<int>(fromBaseValue);
    int toBase = static_cast<int>(toBaseValue);
    if ((fromBase != 2 && fromBase != 10 && fromBase != 16) || (toBase != 2 && toBase != 10 && toBase != 16))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "convert", "bases must be 2, 10, or 16.");
        }
        else
        {
            std::cerr << RED << "Error: bases must be 2, 10, or 16." << RESET << '\n';
        }
        return 2;
    }

    std::string resolvedValue;
    if (!resolveIntegerStringArgument(valueStr, resolvedValue, parseError))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "convert", parseError);
        }
        else
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        return 1;
    }

    try
    {
        long long decimalValue = parseInteger(resolvedValue, fromBase);
        std::string converted = formatInteger(decimalValue, toBase);
        if (outputFormat == OutputFormat::Json)
        {
            std::ostringstream payload;
            payload << "\"fromBase\":" << fromBase << ",\"toBase\":" << toBase;
            payload << ",\"input\":\"" << jsonEscape(resolvedValue) << "\",\"result\":\"" << jsonEscape(converted) << "\"";
            printJsonSuccess(std::cout, "convert", payload.str());
        }
        else
        {
            std::cout << GREEN << "Result: " << RESET << converted << '\n';
        }
        return 0;
    }
    catch (const std::exception &ex)
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "convert", std::string("unable to perform conversion: ") + ex.what());
        }
        else
        {
            std::cerr << RED << "Error: unable to perform conversion: " << ex.what() << RESET << '\n';
        }
        return 1;
    }
}

int runPrimeFactorization(const std::string &input, OutputFormat outputFormat)
{
    long long value = 0;
    std::string parseError;
    if (!resolveIntegerArgument(input, value, parseError))
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cerr, "prime-factorization", parseError);
        }
        else
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        return 1;
    }

    if (value == 0 || value == 1 || value == -1)
    {
        if (outputFormat == OutputFormat::Json)
        {
            std::string noFactorsMessage = std::to_string(value) + " has no prime factors.";
            std::ostringstream payload;
            payload << "\"value\":" << value << ",\"message\":\"" << jsonEscape(noFactorsMessage) << "\"";
            printJsonSuccess(std::cout, "prime-factorization", payload.str());
        }
        else
        {
            std::cout << YELLOW << value << " has no prime factors." << RESET << '\n';
        }
        return 0;
    }

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
        if (outputFormat == OutputFormat::Json)
        {
            std::ostringstream payload;
            payload << "\"value\":" << value << ",\"parts\":[";
            for (std::size_t idx = 0; idx < parts.size(); ++idx)
            {
                if (idx > 0)
                {
                    payload << ',';
                }
                payload << "\"" << jsonEscape(parts[idx]) << "\"";
            }
            payload << "]";
            printJsonSuccess(std::cout, "prime-factorization", payload.str());
        }
        else
        {
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
    }
    catch (const std::exception &ex)
    {
        if (outputFormat == OutputFormat::Json)
        {
            printJsonError(std::cout, "prime-factorization", ex.what());
        }
        else
        {
            std::cout << RED << "Error: " << RESET << ex.what() << '\n';
        }
        return 1;
    }
    return 0;
}

int runHelp(OutputFormat outputFormat)
{
    if (outputFormat == OutputFormat::Json)
    {
        std::string helpText =
            "CLI Calculator Help\n"
            "Usage: calculator [options]\n"
            "Options:\n"
            "  -e, --eval <expression>       Evaluate the given mathematical expression.\n"
            "  -sqrt, --square-root <value>  Calculate the square root of the given value.\n"
            "  -d, --divisors <number>       Calculate and display the divisors of the given number.\n"
            "  -c, --convert <from> <to> <value>  Convert value from one base to another (bases: 2, 10, 16).\n"
            "  -pf, --prime-factorization <value>  Factorize a number into primes.\n"
            "  -b, --batch <file.txt>        Execute CLI flag commands listed in a text file (supports @set/@input/@include/@if/@endif/@unset helpers).\n"
            "  --output json                 Print CLI flag results in JSON.\n"
            "  -nc, --no-color               Disable colored output.\n"
            "  -h, --help                    Display this help message.\n";
        printJsonSuccess(std::cout, "help", "\"text\":\"" + jsonEscape(helpText) + "\"");
    }
    else
    {
        std::cout << BOLD << BLUE << "CLI Calculator Help" << RESET << '\n';
        std::cout << "Usage: calculator [options]\n";
        std::cout << "Options:\n";
        std::cout << "  -e, --eval <expression>       Evaluate the given mathematical expression.\n";
        std::cout << "  -sqrt, --square-root <value>  Calculate the square root of the given value.\n";
        std::cout << "  -d, --divisors <number>       Calculate and display the divisors of the given number.\n";
        std::cout << "  -c, --convert <from> <to> <value>  Convert value from one base to another (bases: 2, 10, 16).\n";
        std::cout << "  -pf, --prime-factorization <value>  Factorize a number into primes.\n";
        std::cout << "  -b, --batch <file.txt>        Execute CLI flag commands listed in a text file (supports @set/@input/@include/@if/@endif/@unset helpers).\n";
        std::cout << "  --output json                 Print CLI flag results in JSON.\n";
        std::cout << "  -nc, --no-color               Disable colored output.\n";
        std::cout << "  -h, --help                    Display this help message.\n";
    }
    return 0;
}
