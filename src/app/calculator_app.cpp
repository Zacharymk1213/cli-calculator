#include "calculator_app.hpp"

#include "ansi_colors.hpp"
#include "cli_batch.hpp"
#include "cli_commands.hpp"
#include "cli_output.hpp"
#include "cli_repl.hpp"
#include "core/variables.hpp"
#include "menu_handlers.hpp"

#include <iostream>

std::optional<int>
CalculatorApp::executeCliAction(const CliParseResult &parseResult) {
  if (!parseResult.action) {
    if (parseResult.outputFormat != OutputFormat::Text &&
        parseResult.sawNonColorArgument) {
      printStructuredError(std::cerr, parseResult.outputFormat, "output",
      "structured output requires a CLI action flag");
      return 1;
    }
    return std::nullopt;
  }
  return dispatchAction(*parseResult.action, parseResult.outputFormat);
}

int CalculatorApp::dispatchAction(const CliAction &action,
                                  OutputFormat format) {
  switch (action.type) {
  case CliActionType::Eval:
    return runEval(action.params.empty() ? "" : action.params.front(), format);
  case CliActionType::SquareRoot:
    return runSquareRoot(action.params.empty() ? "" : action.params.front(),
                         format);
  case CliActionType::Divisors:
    return runDivisors(action.params.empty() ? "" : action.params.front(),
                       format);
  case CliActionType::Convert:
    if (action.params.size() < 3) {
      printStructuredError(std::cerr, format, "convert",
                           "missing arguments after --convert");
      return 2;
    }
    return runConvert(action.params[0], action.params[1], action.params[2],
                      format);
  case CliActionType::PrimeFactorization:
    return runPrimeFactorization(
        action.params.empty() ? "" : action.params.front(), format);
  case CliActionType::Version:
    return runVersion(format);
  case CliActionType::Variables:
    return runListVariables(format);
  case CliActionType::Help:
    return runHelp(format);
  case CliActionType::Repl:
    if (format != OutputFormat::Text) {
      printStructuredError(std::cerr, format, "repl",
                           "REPL mode only supports plain-text output.");
      return 1;
    }
    return runRepl(format);
  case CliActionType::Batch: {
    BatchState batchState;
    if (action.params.empty()) {
      return processBatchFileInteractive(format, batchState);
    }
    return processBatchFile(action.params.front(), format, batchState);
  }
  case CliActionType::None:
    break;
  }
  return 0;
}

int CalculatorApp::run(int argc, char **argv) {
  CliParser parser;
  auto [parseResult, parseError] = parser.parse(argc, argv);

  setColorsEnabled(parseResult.colorsEnabled);

  if (parseError) {
    if (parseResult.outputFormat == OutputFormat::Text) {
      std::cerr << RED << "Error: " << RESET << parseError->message << '\n';
    } else {
      printStructuredError(std::cerr, parseResult.outputFormat,
                           parseError->actionId, parseError->message);
    }
    return parseError->exitCode;
  }

  if (!globalVariableStore().load()) {
    std::cerr << RED
              << "Warning: unable to load vars.toml; variable changes will not "
                 "persist."
              << RESET << '\n';
  }

  if (auto exitCode = executeCliAction(parseResult)) {
    return *exitCode;
  }

  std::cout << BOLD << BLUE << "Welcome to the CLI Calculator!" << RESET
            << '\n';
  runInteractiveCalculator();
  return 0;
}
