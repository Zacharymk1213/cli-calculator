#pragma once

#include <map>
#include <string>

// Parses and evaluates a mathematical expression in infix notation.
double evaluateExpression(const std::string &expression,
                          const std::map<std::string, double> &variables = {});
