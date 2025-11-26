#pragma once

#include <string>

// Converts a textual integer representation in the given base to a decimal value.
long long parseInteger(const std::string &text, int base);

// Formats a decimal integer value into the requested base, adding prefixes for binary/hex.
std::string formatInteger(long long value, int base);
