// CODE IS COMPLETLY REFACTORED AND IMPROVED - 2025/11/26

#include "ansi_colors.hpp"
#include "cli_actions.hpp"
#include "core/variables.hpp"
#include "menu_handlers.hpp"

#include <iostream>
#include <optional>
#include <string>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--no-color" || arg == "-nc")
        {
            setColorsEnabled(false);
            break;
        }
    }

    if (!globalVariableStore().load())
    {
        std::cerr << RED << "Warning: unable to load vars.toml; variable changes will not persist." << RESET << '\n';
    }

    if (auto cliResult = handleCommandLine(argc, argv))
    {
        return *cliResult;
    }

    std::cout << BOLD << BLUE << "Welcome to the CLI Calculator" << RESET << '\n';
    runInteractiveCalculator();
    return 0;
}
