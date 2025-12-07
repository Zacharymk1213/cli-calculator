#include "cli_repl.hpp"

#include "ansi_colors.hpp"
#include "cli_batch.hpp"
#include "cli_commands.hpp"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

namespace
{
    constexpr std::size_t MAX_HISTORY_ENTRIES = 200;

    std::string trim(const std::string &text)
    {
        auto isSpace = [](unsigned char ch)
        { return std::isspace(ch) != 0; };
        auto begin = std::find_if_not(text.begin(), text.end(), isSpace);
        if (begin == text.end())
        {
            return "";
        }
        auto rbegin = std::find_if_not(text.rbegin(), text.rend(), isSpace);
        return {begin, rbegin.base()};
    }

    std::string toLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char ch)
                       { return static_cast<char>(std::tolower(ch)); });
        return value;
    }

    std::string normalizeCommandWord(std::string word)
    {
        auto normalized = toLowerCopy(std::move(word));
        std::replace(normalized.begin(), normalized.end(), '_', '-');
        return normalized;
    }

    std::vector<std::string> splitArgs(const std::string &text)
    {
        std::istringstream stream(text);
        std::vector<std::string> args;
        std::string token;
        while (stream >> token)
        {
            args.push_back(token);
        }
        return args;
    }

    enum class CommandOrigin
    {
        Colon,
        LongOption,
        ShortOption
    };

    enum class CommandKind
    {
        ReplHelp,
        History,
        CliHelp,
        Eval,
        SquareRoot,
        Divisors,
        Convert,
        PrimeFactorization,
        Version,
        Variables,
        Batch,
        NoColor,
        OutputUnsupported,
        AlreadyInRepl
    };

    struct RawCommand
    {
        CommandOrigin origin;
        std::string word;
        std::string args;
    };

    struct ParsedCommand
    {
        CommandKind kind;
        std::string textArgument;
        std::vector<std::string> args;
    };

    bool readLineFallback(const std::string &prompt, std::string &line)
    {
        std::cout << BOLD << GREEN << prompt << RESET;
        std::cout.flush();
        if (!std::getline(std::cin, line))
        {
            return false;
        }
        return true;
    }

#ifndef _WIN32
    bool isInteractiveTerminal()
    {
        return ::isatty(STDIN_FILENO) != 0 && ::isatty(STDOUT_FILENO) != 0;
    }

    bool readLineWithArrowsPosix(const std::string &prompt, const std::vector<std::string> &history, std::string &line)
    {
        if (!isInteractiveTerminal())
        {
            return readLineFallback(prompt, line);
        }

        termios originalSettings;
        if (tcgetattr(STDIN_FILENO, &originalSettings) == -1)
        {
            return readLineFallback(prompt, line);
        }
        termios rawSettings = originalSettings;
        rawSettings.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        rawSettings.c_cc[VMIN] = 1;
        rawSettings.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &rawSettings) == -1)
        {
            return readLineFallback(prompt, line);
        }

        auto restoreTerminal = [&]()
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
        };

        std::string buffer;
        std::string scratchBuffer;
        std::size_t historyIndex = history.size();

        auto renderLine = [&]()
        {
            std::cout << '\r' << BOLD << GREEN << prompt << RESET << buffer << "\033[K";
            std::cout.flush();
        };

        renderLine();

        while (true)
        {
            unsigned char ch = 0;
            ssize_t bytesRead = ::read(STDIN_FILENO, &ch, 1);
            if (bytesRead <= 0)
            {
                restoreTerminal();
                return false;
            }

            if (ch == '\r' || ch == '\n')
            {
                restoreTerminal();
                std::cout << '\n';
                std::cout.flush();
                line = buffer;
                return true;
            }

            if (ch == 0x04) // Ctrl-D
            {
                restoreTerminal();
                return false;
            }

            if (ch == 0x7f || ch == 0x08) // Backspace/Delete
            {
                if (historyIndex != history.size())
                {
                    historyIndex = history.size();
                    buffer = scratchBuffer;
                }
                if (!buffer.empty())
                {
                    buffer.pop_back();
                    scratchBuffer = buffer;
                    renderLine();
                }
                continue;
            }

            if (ch == 0x1b) // Escape - possible arrow key
            {
                unsigned char seq[2] = {0, 0};
                if (::read(STDIN_FILENO, &seq[0], 1) <= 0 || ::read(STDIN_FILENO, &seq[1], 1) <= 0)
                {
                    continue;
                }
                if (seq[0] == '[')
                {
                    if (seq[1] == 'A') // Up
                    {
                        if (history.empty() || historyIndex == 0)
                        {
                            continue;
                        }
                        if (historyIndex == history.size())
                        {
                            scratchBuffer = buffer;
                        }
                        --historyIndex;
                        buffer = history[historyIndex];
                        renderLine();
                    }
                    else if (seq[1] == 'B') // Down
                    {
                        if (historyIndex >= history.size())
                        {
                            continue;
                        }
                        ++historyIndex;
                        if (historyIndex == history.size())
                        {
                            buffer = scratchBuffer;
                        }
                        else
                        {
                            buffer = history[historyIndex];
                        }
                        renderLine();
                    }
                }
                continue;
            }

            if (std::isprint(static_cast<unsigned char>(ch)))
            {
                if (historyIndex != history.size())
                {
                    historyIndex = history.size();
                    buffer = scratchBuffer;
                }
                buffer.push_back(static_cast<char>(ch));
                scratchBuffer = buffer;
                renderLine();
            }
        }
    }
#endif

    bool readLineWithHistoryPrompt(const std::string &prompt, const std::vector<std::string> &history, std::string &line)
    {
#ifndef _WIN32
        return readLineWithArrowsPosix(prompt, history, line);
#else
        (void)history;
        return readLineFallback(prompt, line);
#endif
    }

    std::optional<RawCommand> extractRawCommand(const std::string &input)
    {
        if (input.empty())
        {
            return std::nullopt;
        }
        if (input[0] == ':')
        {
            std::string payload = trim(input.substr(1));
            if (payload.empty())
            {
                return std::nullopt;
            }
            auto delimiter = payload.find_first_of(" \t");
            std::string command = (delimiter == std::string::npos) ? payload : payload.substr(0, delimiter);
            std::string args = (delimiter == std::string::npos) ? "" : trim(payload.substr(delimiter + 1));
            return RawCommand{CommandOrigin::Colon, command, args};
        }
        if (input.rfind("--", 0) == 0)
        {
            auto delimiter = input.find_first_of(" \t");
            std::string command = (delimiter == std::string::npos) ? input.substr(2) : input.substr(2, delimiter - 2);
            std::string args = (delimiter == std::string::npos) ? "" : trim(input.substr(delimiter + 1));
            return RawCommand{CommandOrigin::LongOption, command, args};
        }

        static const std::vector<std::string> SHORT_PREFIXES = {"-e", "-sqrt", "-d", "-c", "-pf", "-v", "-b", "-h", "-nc"};
        for (const auto &prefix : SHORT_PREFIXES)
        {
            if (input.rfind(prefix, 0) == 0)
            {
                std::size_t len = prefix.size();
                if (input.size() == len || std::isspace(static_cast<unsigned char>(input[len])))
                {
                    std::string command = prefix.substr(1);
                    std::string args = (input.size() == len) ? "" : trim(input.substr(len));
                    return RawCommand{CommandOrigin::ShortOption, command, args};
                }
            }
        }

        return std::nullopt;
    }

    std::optional<ParsedCommand> interpretCommand(const RawCommand &raw)
    {
        ParsedCommand parsed;
        parsed.textArgument = raw.args;
        parsed.args = splitArgs(raw.args);

        std::string canonical = normalizeCommandWord(raw.word);
        if (canonical.empty())
        {
            return std::nullopt;
        }

        if (raw.origin == CommandOrigin::Colon)
        {
            if (canonical == "history")
            {
                parsed.kind = CommandKind::History;
                parsed.textArgument.clear();
                parsed.args.clear();
                return parsed;
            }
            if (canonical == "help")
            {
                parsed.kind = CommandKind::ReplHelp;
                parsed.textArgument.clear();
                parsed.args.clear();
                return parsed;
            }
            if (canonical == "cli-help" || canonical == "clihelp" || canonical == "commands")
            {
                parsed.kind = CommandKind::CliHelp;
                parsed.textArgument.clear();
                parsed.args.clear();
                return parsed;
            }
        }

        if (canonical == "eval" || canonical == "e")
        {
            parsed.kind = CommandKind::Eval;
            return parsed;
        }
        if (canonical == "square-root" || canonical == "squareroot" || canonical == "sqrt")
        {
            parsed.kind = CommandKind::SquareRoot;
            return parsed;
        }
        if (canonical == "divisors" || canonical == "d")
        {
            parsed.kind = CommandKind::Divisors;
            return parsed;
        }
        if (canonical == "convert" || canonical == "c")
        {
            parsed.kind = CommandKind::Convert;
            return parsed;
        }
        if (canonical == "prime-factorization" || canonical == "primefactorization" || canonical == "prime" || canonical == "pf")
        {
            parsed.kind = CommandKind::PrimeFactorization;
            return parsed;
        }
        if (canonical == "version" || canonical == "v")
        {
            parsed.kind = CommandKind::Version;
            return parsed;
        }
        if (canonical == "variables" || canonical == "list-variables" || canonical == "listvariables")
        {
            parsed.kind = CommandKind::Variables;
            return parsed;
        }
        if (canonical == "batch" || canonical == "batch-file" || canonical == "batchfile" || canonical == "b")
        {
            parsed.kind = CommandKind::Batch;
            return parsed;
        }
        if (canonical == "help" && raw.origin != CommandOrigin::Colon)
        {
            parsed.kind = CommandKind::CliHelp;
            parsed.textArgument.clear();
            parsed.args.clear();
            return parsed;
        }
        if (canonical == "no-color" || canonical == "nocolor" || canonical == "nc")
        {
            parsed.kind = CommandKind::NoColor;
            parsed.textArgument.clear();
            parsed.args.clear();
            return parsed;
        }
        if (canonical == "output")
        {
            parsed.kind = CommandKind::OutputUnsupported;
            return parsed;
        }
        if (canonical == "repl" || canonical == "r")
        {
            parsed.kind = CommandKind::AlreadyInRepl;
            parsed.textArgument.clear();
            parsed.args.clear();
            return parsed;
        }

        return std::nullopt;
    }

    std::filesystem::path historyFilePath()
    {
        if (const char *overridePath = std::getenv("CLI_CALCULATOR_HISTORY"))
        {
            if (overridePath[0] != '\0')
            {
                return std::filesystem::path(overridePath);
            }
        }

#ifdef _WIN32
        const char *home = std::getenv("USERPROFILE");
#else
        const char *home = std::getenv("HOME");
#endif
        if (home && home[0] != '\0')
        {
            return std::filesystem::path(home) / ".cli-calculator-history";
        }
        return std::filesystem::current_path() / ".cli-calculator-history";
    }

    void appendHistoryEntry(std::vector<std::string> &history, const std::string &entry)
    {
        if (entry.empty())
        {
            return;
        }
        if (!history.empty() && history.back() == entry)
        {
            return;
        }
        history.push_back(entry);
        if (history.size() > MAX_HISTORY_ENTRIES)
        {
            std::size_t toErase = history.size() - MAX_HISTORY_ENTRIES;
            history.erase(history.begin(), history.begin() + static_cast<std::ptrdiff_t>(toErase));
        }
    }

    std::vector<std::string> loadHistory()
    {
        std::vector<std::string> history;
        std::ifstream input(historyFilePath());
        if (!input)
        {
            return history;
        }
        std::string line;
        while (std::getline(input, line))
        {
            appendHistoryEntry(history, trim(line));
        }
        return history;
    }

    void persistHistory(const std::vector<std::string> &history)
    {
        if (history.empty())
        {
            return;
        }
        const auto path = historyFilePath();
        std::error_code ec;
        const auto parent = path.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent, ec);
        }
        std::ofstream output(path, std::ios::trunc);
        if (!output)
        {
            return;
        }
        for (const auto &entry : history)
        {
            output << entry << '\n';
        }
    }

    void printHistory(const std::vector<std::string> &history)
    {
        if (history.empty())
        {
            std::cout << YELLOW << "No history entries yet." << RESET << '\n';
            return;
        }
        std::cout << CYAN << "History:" << RESET << '\n';
        std::size_t index = 1;
        for (const auto &entry : history)
        {
            std::cout << ' ' << index++ << ": " << entry << '\n';
        }
    }

    std::optional<std::string> recallHistoryEntry(const std::vector<std::string> &history, const std::string &command)
    {
        if (command.size() <= 1)
        {
            return std::nullopt;
        }
        std::string_view digits(command.data() + 1, command.size() - 1);
        unsigned long index = 0;
        auto status = std::from_chars(digits.data(), digits.data() + digits.size(), index);
        if (status.ec != std::errc() || status.ptr != digits.data() + digits.size())
        {
            return std::nullopt;
        }
        if (index == 0 || index > history.size())
        {
            return std::nullopt;
        }
        return history[static_cast<std::size_t>(index - 1)];
    }

    void printHelp()
    {
        std::cout << CYAN << "REPL commands:" << RESET << '\n';
        std::cout << "  Type expressions directly (or use ':eval <expr>'/--eval) to evaluate them.\n";
        std::cout << "  ':history' shows stored entries, '!<n>' replays a line, 'exit'/'quit' leaves the REPL.\n";
        std::cout << "  Use the Up/Down arrow keys to browse command history just like in Bash.\n";
        std::cout << "  Every CLI flag works here via ':command' or '--command' (divisors, convert, square-root,\n";
        std::cout << "    prime-factorization, version, variables/list-variables, batch, help/cli-help, etc.).\n";
        std::cout << "  ':cli-help' or '--help' prints the full CLI help text; ':no-color' toggles plain output.\n";
        std::cout << "  Structured formats (--output) are unavailable inside the REPL." << '\n';
    }
} // namespace

int runRepl(OutputFormat outputFormat)
{
    if (outputFormat != OutputFormat::Text)
    {
        std::cerr << RED << "REPL mode only supports plain-text output." << RESET << '\n';
        return 1;
    }

    std::vector<std::string> history = loadHistory();
    std::optional<double> lastResult;

    const std::string prompt = ">>> ";
    std::cout << BOLD << BLUE << "Starting REPL mode. Type ':help' for commands and 'exit' to leave." << RESET << '\n';
    while (true)
    {
        std::string input;
        if (!readLineWithHistoryPrompt(prompt, history, input))
        {
            std::cout << '\n';
            break;
        }

        std::string trimmed = trim(input);
        if (trimmed.empty())
        {
            continue;
        }

        std::string lowered = trimmed;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char ch)
                       { return static_cast<char>(std::tolower(ch)); });

        if (lowered == "exit" || lowered == "quit" || lowered == ":q")
        {
            break;
        }
        if (lowered == "history")
        {
            printHistory(history);
            continue;
        }
        if (lowered == "help")
        {
            printHelp();
            continue;
        }

        if (auto rawCommand = extractRawCommand(trimmed))
        {
            if (auto parsed = interpretCommand(*rawCommand))
            {
                if (parsed->kind != CommandKind::History && parsed->kind != CommandKind::ReplHelp)
                {
                    appendHistoryEntry(history, trimmed);
                }

                switch (parsed->kind)
                {
                case CommandKind::ReplHelp:
                    printHelp();
                    break;
                case CommandKind::History:
                    printHistory(history);
                    break;
                case CommandKind::CliHelp:
                    runHelp(OutputFormat::Text);
                    break;
                case CommandKind::Eval:
                    if (parsed->textArgument.empty())
                    {
                        std::cout << YELLOW << "Usage: :eval <expression> or --eval <expression>" << RESET << '\n';
                        break;
                    }
                    runEval(parsed->textArgument, OutputFormat::Text, &lastResult);
                    break;
                case CommandKind::SquareRoot:
                    if (parsed->args.size() != 1)
                    {
                        std::cout << YELLOW << "Usage: :square-root <value>" << RESET << '\n';
                        break;
                    }
                    runSquareRoot(parsed->args.front(), OutputFormat::Text, &lastResult);
                    break;
                case CommandKind::Divisors:
                    if (parsed->args.size() != 1)
                    {
                        std::cout << YELLOW << "Usage: :divisors <value>" << RESET << '\n';
                        break;
                    }
                    runDivisors(parsed->args.front(), OutputFormat::Text);
                    break;
                case CommandKind::Convert:
                    if (parsed->args.size() != 3)
                    {
                        std::cout << YELLOW << "Usage: :convert <from> <to> <value>" << RESET << '\n';
                        break;
                    }
                    runConvert(parsed->args[0], parsed->args[1], parsed->args[2], OutputFormat::Text);
                    break;
                case CommandKind::PrimeFactorization:
                    if (parsed->args.size() != 1)
                    {
                        std::cout << YELLOW << "Usage: :prime-factorization <value>" << RESET << '\n';
                        break;
                    }
                    runPrimeFactorization(parsed->args.front(), OutputFormat::Text);
                    break;
                case CommandKind::Version:
                    runVersion(OutputFormat::Text);
                    break;
                case CommandKind::Variables:
                    runListVariables(OutputFormat::Text);
                    break;
                case CommandKind::Batch:
                    if (parsed->args.size() != 1)
                    {
                        std::cout << YELLOW << "Usage: :batch <file.txt>" << RESET << '\n';
                        break;
                    }
                    else
                    {
                        BatchState state;
                        state.lastResult = lastResult;
                        (void)processBatchFile(parsed->args.front(), OutputFormat::Text, state);
                        if (state.lastResult)
                        {
                            lastResult = state.lastResult;
                        }
                    }
                    break;
                case CommandKind::NoColor:
                    setColorsEnabled(false);
                    std::cout << YELLOW << "Disabled colored output for this session." << RESET << '\n';
                    break;
                case CommandKind::OutputUnsupported:
                    std::cout << YELLOW << "Structured output (--output) isn't available in REPL mode." << RESET << '\n';
                    break;
                case CommandKind::AlreadyInRepl:
                    std::cout << YELLOW << "You are already inside the REPL." << RESET << '\n';
                    break;
                }
                continue;
            }
        }

        std::string expression = trimmed;
        if (trimmed[0] == '!')
        {
            auto replay = recallHistoryEntry(history, trimmed);
            if (!replay)
            {
                std::cout << YELLOW << "Unknown history entry: " << trimmed << RESET << '\n';
                continue;
            }
            expression = *replay;
            std::cout << CYAN << expression << RESET << '\n';
        }

        appendHistoryEntry(history, expression);
        int status = runEval(expression, OutputFormat::Text, &lastResult);
        if (status != 0)
        {
            continue;
        }
    }

    persistHistory(history);
    return 0;
}
