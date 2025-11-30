#include "cli_numeric.hpp"

#include "core/variables.hpp"

#include <cmath>
#include <limits>

namespace
{
constexpr double IntegerTolerance = 1e-9;
}

bool parseDoubleLiteral(const std::string &token, double &out)
{
    try
    {
        std::size_t idx = 0;
        double value = std::stod(token, &idx);
        if (idx == token.size())
        {
            out = value;
            return true;
        }
    }
    catch (...)
    {
    }
    return false;
}

bool parseLongLongLiteral(const std::string &token, long long &out)
{
    try
    {
        std::size_t idx = 0;
        long long value = std::stoll(token, &idx);
        if (idx == token.size())
        {
            out = value;
            return true;
        }
    }
    catch (...)
    {
    }
    return false;
}

bool convertDoubleToLongLong(double value, long long &out)
{
    if (!std::isfinite(value))
    {
        return false;
    }
    double rounded = std::round(value);
    if (std::fabs(value - rounded) > IntegerTolerance)
    {
        return false;
    }
    if (rounded < static_cast<double>(std::numeric_limits<long long>::min()) ||
        rounded > static_cast<double>(std::numeric_limits<long long>::max()))
    {
        return false;
    }
    out = static_cast<long long>(rounded);
    return true;
}

bool resolveDoubleArgument(const std::string &token, double &out, std::string &error)
{
    if (parseDoubleLiteral(token, out))
    {
        return true;
    }
    if (auto variableValue = globalVariableStore().find(token))
    {
        out = *variableValue;
        return true;
    }
    error = "unable to parse number: " + token;
    return false;
}

bool resolveIntegerArgument(const std::string &token, long long &out, std::string &error)
{
    if (parseLongLongLiteral(token, out))
    {
        return true;
    }
    if (auto variableValue = globalVariableStore().find(token))
    {
        if (convertDoubleToLongLong(*variableValue, out))
        {
            return true;
        }
        error = "variable '" + token + "' must be an integer value";
        return false;
    }
    error = "unable to parse number: " + token;
    return false;
}

bool resolveIntegerStringArgument(const std::string &token, std::string &resolved, std::string &error)
{
    if (auto variableValue = globalVariableStore().find(token))
    {
        long long integerValue = 0;
        if (!convertDoubleToLongLong(*variableValue, integerValue))
        {
            error = "variable '" + token + "' must be an integer value";
            return false;
        }
        resolved = std::to_string(integerValue);
        return true;
    }
    resolved = token;
    return true;
}
