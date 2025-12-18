#pragma once

#include "cli_parser.hpp"

#include <optional>

class CalculatorApp {
public:
  int run(int argc, char **argv);

private:
  std::optional<int> executeCliAction(const CliParseResult &parseResult) const;
  int dispatchAction(const CliAction &action, OutputFormat format) const;
};
