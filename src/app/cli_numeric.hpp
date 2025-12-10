#pragma once

#include <string>

bool parseDoubleLiteral(const std::string &token, double &out);
bool parseLongLongLiteral(const std::string &token, long long &out);
bool convertDoubleToLongLong(double value, long long &out);

bool resolveDoubleArgument(const std::string &token, double &out,
                           std::string &error);
bool resolveIntegerArgument(const std::string &token, long long &out,
                            std::string &error);
bool resolveIntegerStringArgument(const std::string &token,
                                  std::string &resolved, std::string &error);
