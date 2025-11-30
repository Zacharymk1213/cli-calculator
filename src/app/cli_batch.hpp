#pragma once

#include "cli_output.hpp"

#include <optional>
#include <string>
#include <vector>

struct BatchState
{
    std::optional<double> lastResult;
    std::vector<bool> conditionStack;
};

int processBatchFile(const std::string &path, OutputFormat outputFormat, BatchState &state);
