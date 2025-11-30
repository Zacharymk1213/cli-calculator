#pragma once

#include <iosfwd>
#include <string>

enum class OutputFormat
{
    Text,
    Json
};

std::string jsonEscape(const std::string &value);

void printJsonSuccess(std::ostream &os, const std::string &action, const std::string &payload = {});
void printJsonError(std::ostream &os, const std::string &action, const std::string &message);
