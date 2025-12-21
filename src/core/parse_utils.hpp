#pragma once

#include <string>
#include <vector>

std::vector<std::string> parseCsvLine(const std::string &line);
bool parseNumberList(const std::string &input, std::vector<double> &values,
                     std::string &error);
