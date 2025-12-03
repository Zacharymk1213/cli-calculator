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

#ifndef CLI_CALCULATOR_VERSION
#define CLI_CALCULATOR_VERSION "0.1"
#endif

int runEval(const std::string &expression, OutputFormat outputFormat, std::optional<double> *lastResult)
{
    if (lastResult)
    {
        lastResult->reset();
    }
    try
    {
        double result = evaluateExpression(expression, globalVariableStore().variables());
        if (outputFormat == OutputFormat::Text)
        {
            std::cout << GREEN << "Result: " << RESET << result << '\n';
        }
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"expression\":\"" << jsonEscape(expression) << "\",\"result\":" << result;
            std::ostringstream xmlPayload;
            xmlPayload << "<expression>" << xmlEscape(expression) << "</expression><result>" << result << "</result>";
            std::ostringstream yamlPayload;
            yamlPayload << "expression: " << yamlEscape(expression) << '\n'
                        << "result: " << result;
            printStructuredSuccess(std::cout, outputFormat, "eval", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
        }
        if (lastResult)
        {
            *lastResult = result;
        }
        return 0;
    }
    catch (const std::exception &ex)
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cout << RED << "Error: " << RESET << ex.what() << '\n';
        }
        else
        {
            printStructuredError(std::cout, outputFormat, "eval", ex.what());
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
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "square-root", parseError);
        }
        return 1;
    }
    if (value < 0.0)
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: square root undefined for negative values." << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "square-root", "square root undefined for negative values.");
        }
        return 1;
    }
    double result = std::sqrt(value);
    if (outputFormat == OutputFormat::Text)
    {
        std::cout << GREEN << "Result: " << RESET << result << '\n';
    }
    else
    {
        std::ostringstream jsonPayload;
        jsonPayload << "\"value\":" << value << ",\"result\":" << result;
        std::ostringstream xmlPayload;
        xmlPayload << "<value>" << value << "</value><result>" << result << "</result>";
        std::ostringstream yamlPayload;
        yamlPayload << "value: " << value << '\n'
                    << "result: " << result;
        printStructuredSuccess(std::cout, outputFormat, "square-root", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
    }
    if (lastResult)
    {
        *lastResult = result;
    }
    return 0;
}

int runVersion(OutputFormat outputFormat)
{
    constexpr const char *version = CLI_CALCULATOR_VERSION;
    if (outputFormat == OutputFormat::Text)
    {
        std::cout << BOLD << BLUE << "CLI Calculator version " << RESET << version << '\n';
    }
    else
    {
        std::ostringstream jsonPayload;
        jsonPayload << "\"version\":\"" << jsonEscape(version) << "\"";

        std::ostringstream xmlPayload;
        xmlPayload << "<version>" << xmlEscape(version) << "</version>";

        std::ostringstream yamlPayload;
        yamlPayload << "version: " << yamlEscape(version);

        printStructuredSuccess(std::cout, outputFormat, "version", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
    }
    return 0;
}

int runListVariables(OutputFormat outputFormat)
{
    const auto &vars = globalVariableStore().variables();
    if (outputFormat == OutputFormat::Text)
    {
        if (vars.empty())
        {
            std::cout << YELLOW << "No variables stored." << RESET << '\n';
            return 0;
        }

        std::cout << GREEN << "Stored variables:" << RESET << '\n';
        for (const auto &entry : vars)
        {
            std::cout << " - " << entry.first << " = " << entry.second << '\n';
        }
    }
    else
    {
        std::ostringstream jsonPayload;
        jsonPayload << "\"variables\":[";
        bool first = true;
        for (const auto &entry : vars)
        {
            if (!first)
            {
                jsonPayload << ',';
            }
            first = false;
            jsonPayload << "{\"name\":\"" << jsonEscape(entry.first) << "\",\"value\":" << entry.second << "}";
        }
        jsonPayload << ']';

        std::ostringstream xmlPayload;
        xmlPayload << "<variables>";
        for (const auto &entry : vars)
        {
            xmlPayload << "<variable name=\"" << xmlEscape(entry.first) << "\">" << entry.second << "</variable>";
        }
        xmlPayload << "</variables>";

        std::ostringstream yamlPayload;
        yamlPayload << "variables:";
        for (const auto &entry : vars)
        {
            yamlPayload << "\n  - name: " << yamlEscape(entry.first) << "\n    value: " << entry.second;
        }
        if (vars.empty())
        {
            yamlPayload << " []";
        }

        printStructuredSuccess(std::cout, outputFormat, "variables", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
    }
    return 0;
}

int runDivisors(const std::string &input, OutputFormat outputFormat)
{
    long long n = 0;
    std::string parseError;
    if (!resolveIntegerArgument(input, n, parseError))
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "divisors", parseError);
        }
        return 1;
    }

    std::vector<long long> result = calculateDivisors(n);
    if (outputFormat == OutputFormat::Text)
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
    else
    {
        std::ostringstream jsonPayload;
        jsonPayload << "\"number\":" << n << ",\"divisors\":[";
        for (std::size_t idx = 0; idx < result.size(); ++idx)
        {
            if (idx > 0)
            {
                jsonPayload << ',';
            }
            jsonPayload << result[idx];
        }
        jsonPayload << ']';

        std::ostringstream xmlPayload;
        xmlPayload << "<number>" << n << "</number><divisors>";
        for (long long value : result)
        {
            xmlPayload << "<divisor>" << value << "</divisor>";
        }
        xmlPayload << "</divisors>";

        std::ostringstream yamlPayload;
        yamlPayload << "number: " << n << '\n'
                    << "divisors:";
        for (long long value : result)
        {
            yamlPayload << "\n  - " << value;
        }

        printStructuredSuccess(std::cout, outputFormat, "divisors", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
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
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "convert", parseError);
        }
        return 1;
    }
    if (!resolveIntegerArgument(toBaseStr, toBaseValue, parseError))
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "convert", parseError);
        }
        return 1;
    }
    if (fromBaseValue < std::numeric_limits<int>::min() || fromBaseValue > std::numeric_limits<int>::max() ||
        toBaseValue < std::numeric_limits<int>::min() || toBaseValue > std::numeric_limits<int>::max())
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: base is out of supported range." << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "convert", "base is out of supported range.");
        }
        return 1;
    }
    int fromBase = static_cast<int>(fromBaseValue);
    int toBase = static_cast<int>(toBaseValue);
    if ((fromBase != 2 && fromBase != 10 && fromBase != 16) || (toBase != 2 && toBase != 10 && toBase != 16))
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: bases must be 2, 10, or 16." << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "convert", "bases must be 2, 10, or 16.");
        }
        return 2;
    }

    std::string resolvedValue;
    if (!resolveIntegerStringArgument(valueStr, resolvedValue, parseError))
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "convert", parseError);
        }
        return 1;
    }

    try
    {
        long long decimalValue = parseInteger(resolvedValue, fromBase);
        std::string converted = formatInteger(decimalValue, toBase);
        if (outputFormat == OutputFormat::Text)
        {
            std::cout << GREEN << "Result: " << RESET << converted << '\n';
        }
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"fromBase\":" << fromBase << ",\"toBase\":" << toBase;
            jsonPayload << ",\"input\":\"" << jsonEscape(resolvedValue) << "\",\"result\":\"" << jsonEscape(converted) << "\"";

            std::ostringstream xmlPayload;
            xmlPayload << "<fromBase>" << fromBase << "</fromBase><toBase>" << toBase << "</toBase>";
            xmlPayload << "<input>" << xmlEscape(resolvedValue) << "</input><result>" << xmlEscape(converted) << "</result>";

            std::ostringstream yamlPayload;
            yamlPayload << "fromBase: " << fromBase << '\n'
                        << "toBase: " << toBase << '\n'
                        << "input: " << yamlEscape(resolvedValue) << '\n'
                        << "result: " << yamlEscape(converted);

            printStructuredSuccess(std::cout, outputFormat, "convert", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
        }
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::string message = std::string("unable to perform conversion: ") + ex.what();
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << message << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "convert", message);
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
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: " << parseError << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "prime-factorization", parseError);
        }
        return 1;
    }

    if (value == 0 || value == 1 || value == -1)
    {
        std::string noFactorsMessage = std::to_string(value) + " has no prime factors.";
        if (outputFormat == OutputFormat::Text)
        {
            std::cout << YELLOW << value << " has no prime factors." << RESET << '\n';
        }
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"value\":" << value << ",\"message\":\"" << jsonEscape(noFactorsMessage) << "\"";
            std::ostringstream xmlPayload;
            xmlPayload << "<value>" << value << "</value><message>" << xmlEscape(noFactorsMessage) << "</message>";
            std::ostringstream yamlPayload;
            yamlPayload << "value: " << value << '\n'
                        << "message: " << yamlEscape(noFactorsMessage);
            printStructuredSuccess(std::cout, outputFormat, "prime-factorization", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
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
        if (outputFormat == OutputFormat::Text)
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
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"value\":" << value << ",\"parts\":[";
            for (std::size_t idx = 0; idx < parts.size(); ++idx)
            {
                if (idx > 0)
                {
                    jsonPayload << ',';
                }
                jsonPayload << "\"" << jsonEscape(parts[idx]) << "\"";
            }
            jsonPayload << ']';

            std::ostringstream xmlPayload;
            xmlPayload << "<value>" << value << "</value><parts>";
            for (const auto &part : parts)
            {
                xmlPayload << "<part>" << xmlEscape(part) << "</part>";
            }
            xmlPayload << "</parts>";

            std::ostringstream yamlPayload;
            yamlPayload << "value: " << value << '\n'
                        << "parts:";
            for (const auto &part : parts)
            {
                yamlPayload << "\n  - " << yamlEscape(part);
            }

            printStructuredSuccess(std::cout, outputFormat, "prime-factorization", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
        }
    }
    catch (const std::exception &ex)
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cout << RED << "Error: " << RESET << ex.what() << '\n';
        }
        else
        {
            printStructuredError(std::cout, outputFormat, "prime-factorization", ex.what());
        }
        return 1;
    }
    return 0;
}

int runHelp(OutputFormat outputFormat)
{
    const std::string helpText =
        "CLI Calculator Help\n"
        "Usage: calculator [options]\n"
        "Options:\n"
        "  -e, --eval <expression>       Evaluate the given mathematical expression.\n"
        "  -sqrt, --square-root <value>  Calculate the square root of the given value.\n"
        "  -d, --divisors <number>       Calculate and display the divisors of the given number.\n"
        "  -c, --convert <from> <to> <value>  Convert value from one base to another (bases: 2, 10, 16).\n"
        "  -pf, --prime-factorization <value>  Factorize a number into primes.\n"
        "  -v, --version                 Print the application version.\n"
        "  --variables, --list-variables List persisted variables.\n"
        "  -b, --batch <file.txt>        Execute CLI flag commands listed in a text file (supports @set/@input/@include/@if/@endif/@unset helpers).\n"
        "  --output <format>            Print CLI flag results as json, xml, or yaml.\n"
        "  -nc, --no-color               Disable colored output.\n"
        "  -h, --help                    Display this help message.\n";

    if (outputFormat == OutputFormat::Text)
    {
        std::cout << BOLD << BLUE << "CLI Calculator Help" << RESET << '\n';
        std::cout << "Usage: calculator [options]\n";
        std::cout << "Options:\n";
        std::cout << "  -e, --eval <expression>       Evaluate the given mathematical expression.\n";
        std::cout << "  -sqrt, --square-root <value>  Calculate the square root of the given value.\n";
        std::cout << "  -d, --divisors <number>       Calculate and display the divisors of the given number.\n";
        std::cout << "  -c, --convert <from> <to> <value>  Convert value from one base to another (bases: 2, 10, 16).\n";
        std::cout << "  -pf, --prime-factorization <value>  Factorize a number into primes.\n";
        std::cout << "  -v, --version                 Print the application version.\n";
        std::cout << "  --variables, --list-variables List persisted variables.\n";
        std::cout << "  -b, --batch <file.txt>        Execute CLI flag commands listed in a text file (supports @set/@input/@include/@if/@endif/@unset helpers).\n";
        std::cout << "  --output <format>            Print CLI flag results as json, xml, or yaml.\n";
        std::cout << "  -nc, --no-color               Disable colored output.\n";
        std::cout << "  -h, --help                    Display this help message.\n";
    }
    else
    {
        std::string jsonPayload = "\"text\":\"" + jsonEscape(helpText) + "\"";
        std::string xmlPayload = "<text>" + xmlEscape(helpText) + "</text>";
        std::string yamlPayload = std::string("text: ") + yamlEscape(helpText);
        printStructuredSuccess(std::cout, outputFormat, "help", jsonPayload, xmlPayload, yamlPayload);
    }
    return 0;
}
