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

std::string xmlEscape(const std::string &value)
{
    std::string result;
    result.reserve(value.size());
    for (char ch : value)
    {
        switch (ch)
        {
        case '&':
            result += "&amp;";
            break;
        case '<':
            result += "&lt;";
            break;
        case '>':
            result += "&gt;";
            break;
        case '\"':
            result += "&quot;";
            break;
        case '\'':
            result += "&apos;";
            break;
        default:
            result += ch;
        }
    }
    return result;
}

std::string yamlEscape(const std::string &value)
{
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
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
            result.push_back(ch);
        }
    }
    result.push_back('"');
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

void printXmlSuccess(std::ostream &os, const std::string &action, const std::string &payload)
{
    os << "<response action=\"" << xmlEscape(action) << "\" status=\"ok\">";
    if (!payload.empty())
    {
        os << payload;
    }
    os << "</response>\n";
}

void printXmlError(std::ostream &os, const std::string &action, const std::string &message)
{
    os << "<response action=\"" << xmlEscape(action) << "\" status=\"error\"><message>" << xmlEscape(message) << "</message></response>\n";
}

void printYamlSuccess(std::ostream &os, const std::string &action, const std::string &payload)
{
    os << "action: " << yamlEscape(action) << "\nstatus: ok";
    if (!payload.empty())
    {
        os << '\n' << payload;
    }
    os << '\n';
}

void printYamlError(std::ostream &os, const std::string &action, const std::string &message)
{
    os << "action: " << yamlEscape(action) << "\nstatus: error\nmessage: " << yamlEscape(message) << '\n';
}

void printStructuredSuccess(std::ostream &os,
                            OutputFormat format,
                            const std::string &action,
                            const std::string &jsonPayload,
                            const std::string &xmlPayload,
                            const std::string &yamlPayload)
{
    switch (format)
    {
    case OutputFormat::Json:
        printJsonSuccess(os, action, jsonPayload);
        break;
    case OutputFormat::Xml:
        printXmlSuccess(os, action, xmlPayload);
        break;
    case OutputFormat::Yaml:
        printYamlSuccess(os, action, yamlPayload);
        break;
    case OutputFormat::Text:
        break;
    }
}

void printStructuredError(std::ostream &os, OutputFormat format, const std::string &action, const std::string &message)
{
    switch (format)
    {
    case OutputFormat::Json:
        printJsonError(os, action, message);
        break;
    case OutputFormat::Xml:
        printXmlError(os, action, message);
        break;
    case OutputFormat::Yaml:
        printYamlError(os, action, message);
        break;
    case OutputFormat::Text:
        break;
    }
}
