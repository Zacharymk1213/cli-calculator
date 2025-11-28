#include "menu_handlers.hpp"

#include "ansi_colors.hpp"
#include "core/variables.hpp"
#include "divisors.hpp"
#include "equations.hpp"
#include "expression.hpp"
#include "input.hpp"
#include "numeral_conversion.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::string trim(const std::string &text)
    {
        auto notSpace = [](unsigned char ch)
        { return !std::isspace(ch); };
        auto begin = std::find_if(text.begin(), text.end(), notSpace);
        if (begin == text.end())
        {
            return "";
        }
        auto rbegin = std::find_if(text.rbegin(), text.rend(), notSpace);
        return {begin, rbegin.base()};
    }

    int chooseBase(const std::string &label)
    {
        while (true)
        {
            std::cout << label << '\n';
            std::cout << YELLOW << " 1) " << RESET << CYAN << "Decimal (10)" << RESET << '\n';
            std::cout << YELLOW << " 2) " << RESET << CYAN << "Binary (2)" << RESET << '\n';
            std::cout << YELLOW << " 3) " << RESET << CYAN << "Hexadecimal (16)" << RESET << '\n';
            std::cout << YELLOW << " 0) " << RESET << CYAN << "Back" << RESET << '\n';

            int choice = readMenuChoice(0, 3);
            switch (choice)
            {
            case 1:
                return 10;
            case 2:
                return 2;
            case 3:
                return 16;
            case 0:
                return 0;
            default:
                break;
            }
        }
    }

    void printVariables(const std::map<std::string, double> &vars)
    {
        if (vars.empty())
        {
            std::cout << YELLOW << "No variables defined yet." << RESET << '\n';
            return;
        }
        std::cout << CYAN << "Stored variables:" << RESET << '\n';
        int precision = std::numeric_limits<double>::digits10 + 1;
        for (const auto &entry : vars)
        {
            std::ostringstream stream;
            stream << std::setprecision(precision) << entry.second;
            std::cout << "  " << entry.first << " = " << stream.str() << '\n';
        }
    }

    void handleSetVariable(VariableStore &store)
    {
        std::string name = trim(readLine("Variable name to create or update: "));
        if (!VariableStore::isValidName(name))
        {
            std::cout << RED
                      << "Names must start with a letter and may contain letters, digits, or underscores."
                      << RESET << '\n';
            return;
        }
        std::string prompt = "Enter value for '" + name + "': ";
        double value = readDouble(prompt);
        store.set(name, value);
        if (!store.save())
        {
            std::cout << RED << "Unable to persist variables to vars.toml." << RESET << '\n';
            return;
        }
        std::cout << GREEN << "Saved '" << name << "' = " << value << RESET << '\n';
    }

    void handleDeleteVariable(VariableStore &store)
    {
        std::string name = trim(readLine("Variable name to delete: "));
        if (!VariableStore::isValidName(name))
        {
            std::cout << RED
                      << "Names must start with a letter and may contain letters, digits, or underscores."
                      << RESET << '\n';
            return;
        }
        if (!store.remove(name))
        {
            std::cout << YELLOW << "Variable '" << name << "' does not exist." << RESET << '\n';
            return;
        }
        if (!store.save())
        {
            std::cout << RED << "Unable to persist variables to vars.toml." << RESET << '\n';
            return;
        }
        std::cout << GREEN << "Removed variable '" << name << "'." << RESET << '\n';
    }
} // namespace

void handleArithmetic()
{
    while (true)
    {
        std::cout << '\n'
                  << UNDERLINE << MAGENTA << "--- Expression Evaluator ---" << RESET << '\n';
        std::string expression = readLine("Enter an expression (type 'back' to return): ");
        std::string lowered = trim(expression);
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char ch)
                       { return static_cast<char>(std::tolower(ch)); });
        if (lowered == "back")
        {
            return;
        }
        try
        {
            double result = evaluateExpression(expression, globalVariableStore().variables());
            std::cout << GREEN << "Result: " << RESET << result << '\n';
        }
        catch (const std::exception &ex)
        {
            std::cout << RED << "Error: " << RESET << ex.what() << '\n';
        }

        if (!askToContinue("Would you like to evaluate another expression? (y/n): "))
        {
            return;
        }
    }
}

void handleConversions()
{
    while (true)
    {
        std::cout << '\n'
                  << UNDERLINE << BLUE << "--- Numeral System Conversion ---" << RESET << '\n';
        int fromBase = chooseBase(BOLD + YELLOW + std::string("Source base:") + RESET);
        if (fromBase == 0)
        {
            return;
        }
        int toBase = chooseBase(BOLD + BLUE + std::string("Target base:") + RESET);
        if (toBase == 0)
        {
            return;
        }
        if (fromBase == toBase)
        {
            std::cout << RED << "Source and target base are identical; nothing to convert." << RESET << '\n';
            return;
        }

        std::cout << BOLD << BLUE << "Enter the integer to convert: " << RESET;
        std::string rawValue;
        std::cin >> rawValue;

        try
        {
            long long decimalValue = parseInteger(rawValue, fromBase);
            std::string converted = formatInteger(decimalValue, toBase);
            std::cout << GREEN << "Result: " << RESET << converted << '\n';
        }
        catch (const std::exception &ex)
        {
            std::cout << RED << "Error: " << RESET << ex.what() << '\n';
        }

        if (!askToContinue("Would you like to convert another number? (y/n): "))
        {
            return;
        }
    }
}

void handleDivisors()
{
    while (true)
    {
        std::cout << '\n'
                  << UNDERLINE << MAGENTA << "--- Divisor Finder ---" << RESET << '\n';
        long long value = readInteger("Enter an integer (0 allowed): ");
        if (value == 0)
        {
            std::cout << RED << "Zero has infinitely many divisors." << RESET << '\n';
        }
        else
        {
            try
            {
                std::vector<long long> divisors = calculateDivisors(value);
                std::cout << GREEN << "Divisors: " << RESET;
                for (std::size_t idx = 0; idx < divisors.size(); ++idx)
                {
                    if (idx > 0)
                    {
                        std::cout << ", ";
                    }
                    std::cout << divisors[idx];
                }
                std::cout << '\n';
            }
            catch (const std::exception &ex)
            {
                std::cout << RED << "Error: " << RESET << ex.what() << '\n';
            }
        }

        if (!askToContinue("Would you like to check another number? (y/n): "))
        {
            return;
        }
    }
}

void handleEquations()
{
    while (true)
    {
        std::cout << '\n'
                  << UNDERLINE << MAGENTA << "--- Equation Solver ---" << RESET << '\n';
        std::cout << YELLOW << " 1) " << RESET << CYAN << "Linear (a * x + b = 0)" << RESET << '\n';
        std::cout << YELLOW << " 2) " << RESET << CYAN << "Quadratic (a * x^2 + b * x + c = 0)" << RESET << '\n';
        std::cout << YELLOW << " 0) " << RESET << CYAN << "Back" << RESET << '\n';

        int choice = readMenuChoice(0, 2);
        switch (choice)
        {
        case 0:
            return;
        case 1:
        {
            double a = readDouble("Enter coefficient a: ");
            double b = readDouble("Enter coefficient b: ");
            solveLinearEquation(a, b);
            break;
        }
        case 2:
        {
            double a = readDouble("Enter coefficient a: ");
            double b = readDouble("Enter coefficient b: ");
            double c = readDouble("Enter coefficient c: ");
            solveQuadraticEquation(a, b, c);
            break;
        }
        default:
            break;
        }

        if (!askToContinue("Would you like to solve another equation? (y/n): "))
        {
            return;
        }
    }
}

void handleSquareRoot()
{
    while (true)
    {
        std::cout << UNDERLINE << MAGENTA << "\n--- Square Root Calculator ---\n"
                  << RESET;
        double value = readDouble("Enter a number to find its square root: ");

        try
        {
            if (value < 0.0)
            {
                throw std::domain_error("Square root undefined for negative values.");
            }
            double result = std::sqrt(value);
            std::cout << "Square root of " << value << " = " << result << '\n';
        }
        catch (const std::exception &ex)
        {
            std::cout << "Error: " << ex.what() << '\n';
        }

        if (!askToContinue("Would you like to calculate another square root? (y/n): "))
        {
            return;
        }
    }
}

void handleVariables()
{
    auto &store = globalVariableStore();
    while (true)
    {
        std::cout << '\n'
                  << UNDERLINE << BLUE << "--- Variable Manager ---" << RESET << '\n';
        printVariables(store.variables());

        std::cout << YELLOW << " 1) " << RESET << CYAN << "Set or update a variable" << RESET << '\n';
        std::cout << YELLOW << " 2) " << RESET << CYAN << "Delete a variable" << RESET << '\n';
        std::cout << YELLOW << " 0) " << RESET << CYAN << "Back" << RESET << '\n';

        int choice = readMenuChoice(0, 2);
        switch (choice)
        {
        case 0:
            return;
        case 1:
            handleSetVariable(store);
            break;
        case 2:
            handleDeleteVariable(store);
            break;
        default:
            break;
        }
    }
}

void runInteractiveCalculator()
{
    while (true)
    {
        std::cout << '\n'
                  << UNDERLINE << RED << "=== Main Menu ===" << RESET << '\n';
        std::cout << YELLOW << " 1) " << RESET << CYAN << "Basic operations" << RESET << '\n';
        std::cout << YELLOW << " 2) " << RESET << CYAN << "Numeral system conversion" << RESET << '\n';
        std::cout << YELLOW << " 3) " << RESET << CYAN << "Divisor finder" << RESET << '\n';
        std::cout << YELLOW << " 4) " << RESET << CYAN << "Equation solver" << RESET << '\n';
        std::cout << YELLOW << " 5) " << RESET << CYAN << "Square root calculator" << RESET << '\n';
        std::cout << YELLOW << " 6) " << RESET << CYAN << "Variable manager" << RESET << '\n';
        std::cout << YELLOW << " 7) " << RESET << CYAN << "Report a bug" << RESET << '\n';
        std::cout << YELLOW << " 0) " << RESET << CYAN << "Exit" << RESET << '\n';

        int choice = readMenuChoice(0, 7);
        switch (choice)
        {
        case 1:
            handleArithmetic();
            break;
        case 2:
            handleConversions();
            break;
        case 3:
            handleDivisors();
            break;
        case 4:
            handleEquations();
            break;
        case 5:
            handleSquareRoot();
            break;
        case 6:
            handleVariables();
            break;
        case 7:
            std::cout << CYAN << "Opened a browser to report a bug, if don't see it, please visit:" << RESET << '\n';
            std::system("xdg-open https://github.com/Benedek553/cli-calculator/issues");
            std::cout << BLUE << "https://github.com/Benedek553/cli-calculator/issues" << RESET << '\n';
            break;
        case 0:
            std::cout << BOLD << GREEN << "Goodbye!" << RESET << '\n';
            return;
        default:
            break;
        }
    }
}
