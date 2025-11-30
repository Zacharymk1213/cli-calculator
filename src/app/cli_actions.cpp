#include "cli_actions.hpp"

#include "ansi_colors.hpp"
#include "cli_batch.hpp"
#include "cli_commands.hpp"
#include "cli_output.hpp"

#include <iostream>
#include <string>

std::optional<int> handleCommandLine(int argc, char **argv)
{
    OutputFormat outputFormat = OutputFormat::Text;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--no-color" || arg == "-nc")
        {
            continue;
        }
        if (arg == "--output")
        {
            if (i + 1 >= argc)
            {
                std::cerr << RED << "Error: missing format after --output." << RESET << '\n';
                return 1;
            }
            std::string format(argv[i + 1]);
            if (format == "json")
            {
                outputFormat = OutputFormat::Json;
            }
            else
            {
                std::cerr << RED << "Error: unsupported output format: " << format << RESET << '\n';
                return 1;
            }
            ++i;
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--no-color" || arg == "-nc")
        {
            continue;
        }
        if (arg == "--output")
        {
            ++i;
            continue;
        }

        if (arg == "--batch" || arg == "--batch-file" || arg == "-b")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Json)
                {
                    printJsonError(std::cerr, "batch", "missing filename after " + arg);
                }
                else
                {
                    std::cerr << RED << "Error: missing filename after " << arg << RESET << '\n';
                }
                return 1;
            }
            BatchState batchState;
            return processBatchFile(argv[i + 1], outputFormat, batchState);
        }

        if (arg == "--eval" || arg == "-e")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Json)
                {
                    printJsonError(std::cerr, "eval", "missing expression after " + arg);
                }
                else
                {
                    std::cerr << RED << "Error: missing expression after " << arg << RESET << '\n';
                }
                return 1;
            }
            return runEval(argv[i + 1], outputFormat);
        }

        if (arg == "--square-root" || arg == "-sqrt")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Json)
                {
                    printJsonError(std::cerr, "square-root", "missing value after " + arg);
                }
                else
                {
                    std::cerr << RED << "Error: missing value after " << arg << RESET << '\n';
                }
                return 1;
            }
            return runSquareRoot(argv[i + 1], outputFormat);
        }

        if (arg == "--divisors" || arg == "-d")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Json)
                {
                    printJsonError(std::cerr, "divisors", "missing value after " + arg);
                }
                else
                {
                    std::cerr << RED << "Error: Missing value after " << arg << RESET << '\n';
                }
                return 2;
            }
            return runDivisors(argv[i + 1], outputFormat);
        }

        if (arg == "--convert" || arg == "-c")
        {
            if (i + 3 >= argc)
            {
                if (outputFormat == OutputFormat::Json)
                {
                    printJsonError(std::cerr, "convert", "missing arguments after " + arg);
                }
                else
                {
                    std::cerr << RED << "Error: missing arguments after " << arg << RESET << '\n';
                }
                return 2;
            }
            return runConvert(argv[i + 1], argv[i + 2], argv[i + 3], outputFormat);
        }
        if (arg == "--help" || arg == "-h")
        {
            return runHelp(outputFormat);
        }
        if (arg == "--prime-factorization" || arg == "-pf")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Json)
                {
                    printJsonError(std::cerr, "prime-factorization", "missing arguments after " + arg);
                }
                else
                {
                    std::cerr << RED << "Error: missing arguments after " << arg << RESET << '\n';
                }
                return 2;
            }
            return runPrimeFactorization(argv[i + 1], outputFormat);
        }
    }

    return std::nullopt;
}
