#include "cli_batch.hpp"

#include "ansi_colors.hpp"
#include "cli_commands.hpp"
#include "core/variables.hpp"
#include "expression.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
std::string trimCopy(const std::string &value)
{
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }
    return value.substr(start, end - start);
}

std::vector<std::string> splitTokens(const std::string &line)
{
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

std::string joinTokens(const std::vector<std::string> &tokens, std::size_t start)
{
    if (start >= tokens.size())
    {
        return {};
    }
    std::ostringstream joined;
    for (std::size_t idx = start; idx < tokens.size(); ++idx)
    {
        if (idx > start)
        {
            joined << ' ';
        }
        joined << tokens[idx];
    }
    return joined.str();
}

std::string normalizeConditionExpression(const std::string &expression)
{
    std::string normalized;
    normalized.reserve(expression.size() + 4);
    for (std::size_t idx = 0; idx < expression.size(); ++idx)
    {
        char ch = expression[idx];
        if (ch == '=')
        {
            char prev = idx > 0 ? expression[idx - 1] : '\0';
            char next = (idx + 1 < expression.size()) ? expression[idx + 1] : '\0';
            if (prev != '!' && prev != '<' && prev != '>' && prev != '=' && next != '=')
            {
                normalized += "==";
                continue;
            }
        }
        normalized.push_back(ch);
    }
    return normalized;
}

bool hasTxtExtension(const std::string &path)
{
    auto dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return false;
    }
    std::string extension = path.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension == ".txt";
}

std::string normalizeBatchFlag(const std::string &flag)
{
    if (!flag.empty() && flag[0] == '@')
    {
        std::string command;
        command.reserve(flag.size());
        for (std::size_t idx = 1; idx < flag.size(); ++idx)
        {
            command.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(flag[idx]))));
        }
        if (command == "set")
        {
            return "@set";
        }
        if (command == "input")
        {
            return "@input";
        }
        if (command == "include")
        {
            return "@include";
        }
        if (command == "if")
        {
            return "@if";
        }
        if (command == "endif")
        {
            return "@endif";
        }
        if (command == "unset")
        {
            return "@unset";
        }
        return {};
    }

    std::string stripped;
    stripped.reserve(flag.size());
    std::size_t idx = 0;
    while (idx < flag.size() && flag[idx] == '-')
    {
        ++idx;
    }
    for (; idx < flag.size(); ++idx)
    {
        stripped.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(flag[idx]))));
    }

    if (stripped.empty())
    {
        return {};
    }
    if (stripped == "e" || stripped == "eval")
    {
        return "--eval";
    }
    if (stripped == "sqrt" || stripped == "squareroot" || stripped == "square-root")
    {
        return "--square-root";
    }
    if (stripped == "d" || stripped == "divisors")
    {
        return "--divisors";
    }
    if (stripped == "c" || stripped == "convert")
    {
        return "--convert";
    }
    if (stripped == "pf" || stripped == "primefactorization" || stripped == "prime-factorization")
    {
        return "--prime-factorization";
    }
    if (stripped == "h" || stripped == "help")
    {
        return "--help";
    }
    return {};
}

bool batchExecutionEnabled(const BatchState &state)
{
    for (bool cond : state.conditionStack)
    {
        if (!cond)
        {
            return false;
        }
    }
    return true;
}

int dispatchBatchCommand(const std::vector<std::string> &tokens,
                         OutputFormat outputFormat,
                         BatchState &state,
                         const std::filesystem::path &currentDir)
{
    if (tokens.empty())
    {
        return 0;
    }
    const std::string &flag = tokens[0];
    if (flag == "--eval")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing expression after --eval" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "eval", "missing expression after --eval");
            }
            return 1;
        }
        return runEval(joinTokens(tokens, 1), outputFormat, &state.lastResult);
    }
    if (flag == "--square-root")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing value after --square-root" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "square-root", "missing value after --square-root");
            }
            return 1;
        }
        return runSquareRoot(tokens[1], outputFormat, &state.lastResult);
    }
    if (flag == "--divisors")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: Missing value after --divisors" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "divisors", "missing value after --divisors");
            }
            return 2;
        }
        state.lastResult.reset();
        return runDivisors(tokens[1], outputFormat);
    }
    if (flag == "--convert")
    {
        if (tokens.size() < 4)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing arguments after --convert" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "convert", "missing arguments after --convert");
            }
            return 2;
        }
        if (tokens.size() > 4)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unexpected extra tokens after --convert arguments" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "convert", "unexpected extra tokens after --convert arguments");
            }
            return 2;
        }
        state.lastResult.reset();
        return runConvert(tokens[1], tokens[2], tokens[3], outputFormat);
    }
    if (flag == "--prime-factorization")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing arguments after --prime-factorization" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "prime-factorization", "missing arguments after --prime-factorization");
            }
            return 2;
        }
        state.lastResult.reset();
        return runPrimeFactorization(tokens[1], outputFormat);
    }
    if (flag == "--help")
    {
        state.lastResult.reset();
        return runHelp(outputFormat);
    }

    if (flag == "@set")
    {
        if (tokens.size() < 2)
        {
            state.lastResult.reset();
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing variable name after @set" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "set", "missing variable name after @set");
            }
            return 1;
        }
        if (!state.lastResult)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: no numeric result available to store." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "set", "no numeric result available to store");
            }
            return 1;
        }
        const std::string &variableName = tokens[1];
        if (!VariableStore::isValidName(variableName))
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: invalid variable name: " << variableName << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "set", "invalid variable name: " + variableName);
            }
            return 1;
        }
        globalVariableStore().set(variableName, *state.lastResult);
        if (!globalVariableStore().save())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unable to persist variable store." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "set", "unable to persist variable store");
            }
            return 1;
        }
        if (outputFormat == OutputFormat::Text)
        {
            std::cout << GREEN << "Stored last result in variable '" << variableName << "'." << RESET << '\n';
        }
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"variable\":\"" << jsonEscape(variableName) << "\",\"value\":" << *state.lastResult;
            std::ostringstream xmlPayload;
            xmlPayload << "<variable>" << xmlEscape(variableName) << "</variable><value>" << *state.lastResult << "</value>";
            std::ostringstream yamlPayload;
            yamlPayload << "variable: " << yamlEscape(variableName) << '\n'
                        << "value: " << *state.lastResult;
            printStructuredSuccess(std::cout, outputFormat, "set", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
        }
        return 0;
    }

    if (flag == "@input")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing variable name after @input" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "input", "missing variable name after @input");
            }
            return 1;
        }
        const std::string &variableName = tokens[1];
        if (!VariableStore::isValidName(variableName))
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: invalid variable name: " << variableName << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "input", "invalid variable name: " + variableName);
            }
            return 1;
        }

        std::cout << CYAN << "Enter value for '" << variableName << "': " << RESET << std::flush;
        std::string line;
        if (!std::getline(std::cin, line))
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: failed to read value for variable '" << variableName << "'." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "input", "failed to read value for variable: " + variableName);
            }
            return 1;
        }
        std::string trimmed = trimCopy(line);
        if (trimmed.empty())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: no value entered for variable '" << variableName << "'." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "input", "no value entered for variable: " + variableName);
            }
            return 1;
        }

        double value = 0.0;
        try
        {
            value = evaluateExpression(trimmed, globalVariableStore().variables());
        }
        catch (const std::exception &ex)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unable to evaluate value: " << ex.what() << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "input", "unable to evaluate value: " + std::string(ex.what()));
            }
            return 1;
        }

        globalVariableStore().set(variableName, value);
        if (!globalVariableStore().save())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unable to persist variable store." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "input", "unable to persist variable store");
            }
            return 1;
        }

        if (outputFormat == OutputFormat::Text)
        {
            std::cout << GREEN << "Stored input in variable '" << variableName << "'." << RESET << '\n';
        }
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"variable\":\"" << jsonEscape(variableName) << "\",\"value\":" << value;
            std::ostringstream xmlPayload;
            xmlPayload << "<variable>" << xmlEscape(variableName) << "</variable><value>" << value << "</value>";
            std::ostringstream yamlPayload;
            yamlPayload << "variable: " << yamlEscape(variableName) << '\n'
                        << "value: " << value;
            printStructuredSuccess(std::cout, outputFormat, "input", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
        }
        return 0;
    }
    if (flag == "@include")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing filename after @include" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "include", "missing filename after @include");
            }
            return 1;
        }
        std::filesystem::path includePath(tokens[1]);
        if (includePath.is_relative())
        {
            includePath = currentDir / includePath;
        }
        includePath = includePath.lexically_normal();
        return processBatchFile(includePath.string(), outputFormat, state);
    }
    if (flag == "@unset")
    {
        if (tokens.size() < 2)
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing variable name after @unset" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "unset", "missing variable name after @unset");
            }
            return 1;
        }
        const std::string &variableName = tokens[1];
        if (!VariableStore::isValidName(variableName))
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: invalid variable name: " << variableName << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "unset", "invalid variable name: " + variableName);
            }
            return 1;
        }
        bool removed = globalVariableStore().remove(variableName);
        if (!globalVariableStore().save())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unable to persist variable store." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "unset", "unable to persist variable store");
            }
            return 1;
        }
        if (outputFormat == OutputFormat::Text)
        {
            if (removed)
            {
                std::cout << GREEN << "Removed variable '" << variableName << "'." << RESET << '\n';
            }
            else
            {
                std::cout << YELLOW << "Variable '" << variableName << "' did not exist." << RESET << '\n';
            }
        }
        else
        {
            std::ostringstream jsonPayload;
            jsonPayload << "\"variable\":\"" << jsonEscape(variableName) << "\",\"removed\":" << (removed ? "true" : "false");
            std::ostringstream xmlPayload;
            xmlPayload << "<variable>" << xmlEscape(variableName) << "</variable><removed>" << (removed ? "true" : "false")
                       << "</removed>";
            std::ostringstream yamlPayload;
            yamlPayload << "variable: " << yamlEscape(variableName) << '\n'
                        << "removed: " << (removed ? "true" : "false");
            printStructuredSuccess(std::cout, outputFormat, "unset", jsonPayload.str(), xmlPayload.str(), yamlPayload.str());
        }
        return 0;
    }
    if (flag == "@if")
    {
        if (tokens.size() < 2)
        {
            state.conditionStack.push_back(false);
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: missing expression after @if" << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "if", "missing expression after @if");
            }
            return 1;
        }
        if (!batchExecutionEnabled(state))
        {
            state.conditionStack.push_back(false);
            return 0;
        }
        std::string conditionExpr = normalizeConditionExpression(joinTokens(tokens, 1));
        try
        {
            double conditionValue = evaluateExpression(conditionExpr, globalVariableStore().variables());
            state.conditionStack.push_back(conditionValue != 0.0);
            return 0;
        }
        catch (const std::exception &ex)
        {
            state.conditionStack.push_back(false);
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: " << ex.what() << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "if", ex.what());
            }
            return 1;
        }
    }
    if (flag == "@endif")
    {
        if (state.conditionStack.empty())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: @endif without matching @if." << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "endif", "@endif without matching @if");
            }
            return 1;
        }
        state.conditionStack.pop_back();
        return 0;
    }

    if (outputFormat == OutputFormat::Text)
    {
        std::cerr << RED << "Error: unsupported command: " << flag << RESET << '\n';
    }
    else
    {
        printStructuredError(std::cerr, outputFormat, "batch", "unsupported command: " + flag);
    }
    state.lastResult.reset();
    return 1;
}
} // namespace

int processBatchFile(const std::string &path, OutputFormat outputFormat, BatchState &state)
{
    if (!hasTxtExtension(path))
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: batch processing expects a .txt file." << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "batch", "batch processing expects a .txt file");
        }
        return 1;
    }

    std::ifstream input(path);
    if (!input)
    {
        if (outputFormat == OutputFormat::Text)
        {
            std::cerr << RED << "Error: unable to open batch file: " << path << RESET << '\n';
        }
        else
        {
            printStructuredError(std::cerr, outputFormat, "batch", "unable to open batch file: " + path);
        }
        return 1;
    }

    bool anyFailure = false;
    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (directory.empty())
    {
        directory = std::filesystem::current_path();
    }
    std::string line;
    while (std::getline(input, line))
    {
        std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed[0] == '#')
        {
            continue;
        }
        auto tokens = splitTokens(trimmed);
        if (tokens.empty())
        {
            continue;
        }
        std::string normalized = normalizeBatchFlag(tokens[0]);
        if (normalized.empty())
        {
            if (outputFormat == OutputFormat::Text)
            {
                std::cerr << RED << "Error: unsupported command: " << tokens[0] << RESET << '\n';
            }
            else
            {
                printStructuredError(std::cerr, outputFormat, "batch", "unsupported command: " + tokens[0]);
            }
            anyFailure = true;
            continue;
        }
        tokens[0] = normalized;

        bool isConditionalDirective = (normalized == "@if" || normalized == "@endif");
        if (!isConditionalDirective && !batchExecutionEnabled(state))
        {
            continue;
        }

        int result = dispatchBatchCommand(tokens, outputFormat, state, directory);
        if (result != 0)
        {
            anyFailure = true;
        }
    }
    return anyFailure ? 1 : 0;
}
