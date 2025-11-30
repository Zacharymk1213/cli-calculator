#include "cli_output.hpp"

#include <ostream>

std::string jsonEscape(const std::string &value)
{
    std::string result;
    result.reserve(value.size());
    for (char ch : value)
    {
        switch (ch)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20)
            {
                static constexpr char digits[] = "0123456789ABCDEF";
                unsigned char uc = static_cast<unsigned char>(ch);
                result += "\\u00";
                result += digits[(uc >> 4) & 0xF];
                result += digits[uc & 0xF];
            }
            else
            {
                result += ch;
            }
        }
    }
    return result;
}

void printJsonSuccess(std::ostream &os, const std::string &action, const std::string &payload)
{
    os << "{\"action\":\"" << action << "\",\"status\":\"ok\"";
    if (!payload.empty())
    {
        os << ',' << payload;
    }
    os << "}\n";
}

void printJsonError(std::ostream &os, const std::string &action, const std::string &message)
{
    os << "{\"action\":\"" << action << "\",\"status\":\"error\",\"message\":\"" << jsonEscape(message) << "\"}\n";
}
