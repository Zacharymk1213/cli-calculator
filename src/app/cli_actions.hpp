#pragma once

#include <optional>

// Processes single-shot CLI flags (eval/convert/etc). Returns an exit code if
// a flag handled the request, or std::nullopt to fall back to interactive mode.
std::optional<int> handleCommandLine(int argc, char **argv);
