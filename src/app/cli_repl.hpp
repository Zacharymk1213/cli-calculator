#pragma once

#include "cli_output.hpp"

// Launches the expression REPL. Returns 0 on clean exit, non-zero on failure.
int runRepl(OutputFormat outputFormat);
