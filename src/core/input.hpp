#pragma once
#include <string>

// Common input helpers used by the interactive menus.
void clearInput();
std::string readLine(const std::string &prompt);
int readMenuChoice(int min, int max);
long long readInteger(const std::string &prompt);
double readDouble(const std::string &prompt);
bool askToContinue(const std::string &prompt);
