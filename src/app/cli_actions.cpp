#include "cli_actions.hpp"
#include "ansi_colors.hpp"
#include "cli_batch.hpp"
#include "cli_commands.hpp"
#include "cli_output.hpp"
#include "cli_repl.hpp"
#include <iostream>
#include <string>

std::optional<int> handleCommandLine(int argc, char **argv)
{
    OutputFormat outputFormat = OutputFormat::Text;
    bool hasNonColorArgs = false;

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
            else if (format == "xml")
            {
                outputFormat = OutputFormat::Xml;
            }
            else if (format == "yaml" || format == "yml")
            {
                outputFormat = OutputFormat::Yaml;
            }
            else
            {
                std::cerr << RED << "Error: unsupported output format: " << format << RESET << '\n';
                return 1;
            }
            ++i;
            hasNonColorArgs = true;
        }
        else
        {
            hasNonColorArgs = true;
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
                if (outputFormat == OutputFormat::Text)
                {
                    std::cerr << RED << "Error: missing filename after " << arg << RESET << '\n';
                }
                else
                {
                    printStructuredError(std::cerr, outputFormat, "batch", "missing filename after " + arg);
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
                if (outputFormat == OutputFormat::Text)
                {
                    std::cerr << RED << "Error: missing expression after " << arg << RESET << '\n';
                }
                else
                {
                    printStructuredError(std::cerr, outputFormat, "eval", "missing expression after " + arg);
                }
                return 1;
            }
            return runEval(argv[i + 1], outputFormat);
        }

        if (arg == "--square-root" || arg == "-sqrt")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Text)
                {
                    std::cerr << RED << "Error: missing value after " << arg << RESET << '\n';
                }
                else
                {
                    printStructuredError(std::cerr, outputFormat, "square-root", "missing value after " + arg);
                }
                return 1;
            }
            return runSquareRoot(argv[i + 1], outputFormat);
        }

        if (arg == "--divisors" || arg == "-d")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Text)
                {
                    std::cerr << RED << "Error: Missing value after " << arg << RESET << '\n';
                }
                else
                {
                    printStructuredError(std::cerr, outputFormat, "divisors", "missing value after " + arg);
                }
                return 2;
            }
            return runDivisors(argv[i + 1], outputFormat);
        }

        if (arg == "--convert" || arg == "-c")
        {
            if (i + 3 >= argc)
            {
                if (outputFormat == OutputFormat::Text)
                {
                    std::cerr << RED << "Error: missing arguments after " << arg << RESET << '\n';
                }
                else
                {
                    printStructuredError(std::cerr, outputFormat, "convert", "missing arguments after " + arg);
                }
                return 2;
            }
            return runConvert(argv[i + 1], argv[i + 2], argv[i + 3], outputFormat);
        }
        if (arg == "--help" || arg == "-h")
        {
            return runHelp(outputFormat);
        }
        if (arg == "--repl" || arg == "-r")
        {
            if (outputFormat != OutputFormat::Text)
            {
                printStructuredError(std::cerr, outputFormat, "repl", "REPL mode only supports plain-text output.");
                return 1;
            }
            return runRepl(outputFormat);
        }
        if (arg == "--prime-factorization" || arg == "-pf")
        {
            if (i + 1 >= argc)
            {
                if (outputFormat == OutputFormat::Text)
                {
                    std::cerr << RED << "Error: missing arguments after " << arg << RESET << '\n';
                }
                else
                {
                    printStructuredError(std::cerr, outputFormat, "prime-factorization", "missing arguments after " + arg);
                }
                return 2;
            }
            return runPrimeFactorization(argv[i + 1], outputFormat);
        }

        if (arg == "--version" || arg == "-v")
        {
            return runVersion(outputFormat);
        }

        if (arg == "--variables" || arg == "--list-variables")
        {
            return runListVariables(outputFormat);
        }

        if (!arg.empty())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unknown argument: " << arg << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "unknown-argument", "unknown argument: " + arg);
            }
            return 1;
        }
    }

    if (outputFormat != OutputFormat::Text && hasNonColorArgs)
    {
        printStructuredError(std::cerr, outputFormat, "output", "structured output requires a CLI action flag");
        return 1;
    }

    return std::nullopt;
}
