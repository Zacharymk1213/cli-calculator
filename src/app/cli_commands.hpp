#pragma once

#include "cli_output.hpp"

#include <optional>
#include <string>

int runEval(const std::string &expression, OutputFormat outputFormat, std::optional<double> *lastResult = nullptr);
int runSquareRoot(const std::string &number, OutputFormat outputFormat, std::optional<double> *lastResult = nullptr);
int runDivisors(const std::string &input, OutputFormat outputFormat);
int runConvert(const std::string &fromBaseStr, const std::string &toBaseStr, const std::string &valueStr, OutputFormat outputFormat);
int runPrimeFactorization(const std::string &input, OutputFormat outputFormat);
int runVersion(OutputFormat outputFormat);
int runListVariables(OutputFormat outputFormat);
int runHelp(OutputFormat outputFormat);
