#include "input.hpp"

#include "ansi_colors.hpp"

#include <cctype>
#include <iostream>
#include <limits>
#include <string>

namespace
{
    void printPrompt(const std::string &prompt)
    {
        std::cout << BOLD << BLUE << prompt << RESET;
    }
} // namespace

void clearInput()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string readLine(const std::string &prompt)
{
    printPrompt(prompt);
    std::cin >> std::ws;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int readMenuChoice(int min, int max)
{
    int choice;
    while (true)
    {
        std::cout << BOLD << CYAN << "Select option: " << RESET;
        if (std::cin >> choice && choice >= min && choice <= max)
        {
            return choice;
        }
        std::cout << RED << "Invalid choice. Try again." << RESET << '\n';
        clearInput();
    }
}

long long readInteger(const std::string &prompt)
{
    long long value;
    while (true)
    {
        printPrompt(prompt);
        if (std::cin >> value)
        {
            return value;
        }
        std::cout << RED << "That doesn't look like an integer." << RESET << ' ';
        clearInput();
    }
}

double readDouble(const std::string &prompt)
{
    double value;
    while (true)
    {
        printPrompt(prompt);
        if (std::cin >> value)
        {
            return value;
        }
        std::cout << RED << "That doesn't look like a valid number." << RESET << ' ';
        clearInput();
    }
}

bool askToContinue(const std::string &prompt)
{
    std::string answer;
    while (true)
    {
        std::cout << BOLD << BLUE << prompt << RESET;
        if (!(std::cin >> answer))
        {
            clearInput();
            continue;
        }
        if (answer.empty())
        {
            continue;
        }

        char first = static_cast<char>(std::tolower(static_cast<unsigned char>(answer[0])));
        if (first == 'y')
        {
            return true;
        }
        if (first == 'n')
        {
            return false;
        }
        std::cout << YELLOW << "Please answer with 'y' or 'n'." << RESET << '\n';
    }
}
