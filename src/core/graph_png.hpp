#pragma once
#include <string>
#include <vector>

// Renders the provided numeric values into a PNG image stored at
// outputPath. The function returns true on success and writes any failure
// details into errorMessage.
bool generateGraphPng(const std::vector<double> &values,
                      const std::string &outputPath,
                      std::string &errorMessage);
