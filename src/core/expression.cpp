#include "expression.hpp"

#include "math_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    bool isOperatorChar(char ch)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return ch == '+' || ch == '-' || ch == '*' || ch == 'x' || ch == ':' || ch == '/';
    }

    char normalizeOperator(char ch)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (ch == 'x')
        {
            return '*';
        }
        if (ch == ':')
        {
            return '/';
        }
        return ch;
    }

    double factorialOf(double operand)
    {
        double rounded = std::round(operand);
        if (!isApproximatelyZero(operand - rounded))
        {
            throw std::invalid_argument("Factorial is only defined for integers.");
        }

        long long n = static_cast<long long>(rounded);
        if (n < 0)
        {
            throw std::invalid_argument("Factorial is not defined for negative numbers.");
        }
        if (n > 170)
        {
            throw std::overflow_error("Factorial result would overflow double precision.");
        }

        long double result = 1.0L;
        for (long long i = 2; i <= n; ++i)
        {
            result *= static_cast<long double>(i);
        }
        return static_cast<double>(result);
    }

    double applyFunction(const std::string &functionName, double value)
    {
        if (functionName == "sin")
        {
            return std::sin(value);
        }
        if (functionName == "cos")
        {
            return std::cos(value);
        }
        if (functionName == "log")
        {
            if (value <= 0.0)
            {
                throw std::domain_error("Logarithm undefined for non-positive values.");
            }
            return std::log(value);
        }
        if (functionName == "tan")
        {
            return std::tan(value);
        }
        if (functionName == "sqrt")
        {
            if (value < 0.0)
            {
                throw std::domain_error("Square root undefined for negative values.");
            }
            return std::sqrt(value);
        }
        if (functionName == "exp")
        {
            return std::exp(value);
        }
        if (functionName == "cot")
        {
            double tanValue = std::tan(value);
            if (isApproximatelyZero(tanValue))
            {
                throw std::domain_error("Cotangent undefined for this value.");
            }
            return 1.0 / tanValue;
        }
        if (functionName == "asin")
        {
            if (value < -1.0 || value > 1.0)
            {
                throw std::domain_error("Arcsine undefined for this value.");
            }
            return std::asin(value);
        }
        if (functionName == "acos")
        {
            if (value < -1.0 || value > 1.0)
            {
                throw std::domain_error("Arccosine undefined for this value.");
            }
            return std::acos(value);
        }
        if (functionName == "atan")
        {
            return std::atan(value);
        }
        if (functionName == "sinh")
        {
            return std::sinh(value);
        }
        throw std::invalid_argument("Unknown function: " + functionName);
    }

    struct Token
    {
        enum class Type
        {
            Number,
            Operator,
            Function,
            Variable,
            LeftParen,
            RightParen
        } type;
        double value{};
        char op{};
        std::string text;
    };

    double parseNumberToken(const std::string &expr, std::size_t &index)
    {
        std::size_t start = index;
        bool hasDigit = false;
        bool hasDot = false;
        while (index < expr.size())
        {
            char c = expr[index];
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                hasDigit = true;
                ++index;
            }
            else if (c == '.')
            {
                if (hasDot)
                {
                    throw std::invalid_argument("Multiple decimal separators found in number.");
                }
                hasDot = true;
                ++index;
            }
            else
            {
                break;
            }
        }
        if (!hasDigit)
        {
            throw std::invalid_argument("Expected a digit in the number.");
        }
        return std::stod(expr.substr(start, index - start));
    }

    std::vector<Token> tokenizeExpression(const std::string &expression)
    {
        std::vector<Token> tokens;
        std::size_t i = 0;
        bool expectValue = true;

        while (i < expression.size())
        {
            char c = expression[i];
            if (std::isspace(static_cast<unsigned char>(c)))
            {
                ++i;
                continue;
            }

            if (expectValue)
            {
                if (c == '(')
                {
                    tokens.push_back({Token::Type::LeftParen, 0.0, 0, ""});
                    ++i;
                    continue;
                }

                int sign = 1;
                bool sawUnarySign = false;
                if (c == '+' || c == '-')
                {
                    sawUnarySign = true;
                    sign = (c == '-') ? -1 : 1;
                    ++i;
                    while (i < expression.size() &&
                           std::isspace(static_cast<unsigned char>(expression[i])))
                    {
                        ++i;
                    }
                    if (i >= expression.size())
                    {
                        throw std::invalid_argument("Expression cannot end with a unary operator.");
                    }
                    c = expression[i];
                    if (c == '(')
                    {
                        if (sign == -1)
                        {
                            tokens.push_back({Token::Type::Number, 0.0, 0, ""});
                            tokens.push_back({Token::Type::Operator, 0.0, '-', ""});
                        }
                        expectValue = true;
                        continue;
                    }
                }
                else
                {
                    sign = 1;
                }

                if (std::isdigit(static_cast<unsigned char>(c)) || c == '.')
                {
                    double number = parseNumberToken(expression, i);
                    tokens.push_back({Token::Type::Number, sign * number, 0, ""});
                    expectValue = false;
                    continue;
                }

                if (std::isalpha(static_cast<unsigned char>(c)))
                {
                    std::size_t start = i;
                    ++i;
                    while (i < expression.size() &&
                           (std::isalnum(static_cast<unsigned char>(expression[i])) || expression[i] == '_'))
                    {
                        ++i;
                    }
                    std::string identifier = expression.substr(start, i - start);
                    std::string lowered = identifier;
                    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch)
                                   { return static_cast<char>(std::tolower(ch)); });

                    if (lowered == "sin" || lowered == "cos" || lowered == "log" || lowered == "tan" ||
                        lowered == "sqrt" || lowered == "exp" || lowered == "cot" || lowered == "asin" ||
                        lowered == "acos" || lowered == "atan" || lowered == "sinh")
                    {
                        if (sawUnarySign && sign == -1)
                        {
                            tokens.push_back({Token::Type::Number, 0.0, 0, ""});
                            tokens.push_back({Token::Type::Operator, 0.0, '-', ""});
                        }
                        tokens.push_back({Token::Type::Function, 0.0, 0, lowered});

                        std::size_t lookahead = i;
                        while (lookahead < expression.size() &&
                               std::isspace(static_cast<unsigned char>(expression[lookahead])))
                        {
                            ++lookahead;
                        }
                        if (lookahead >= expression.size() || expression[lookahead] != '(')
                        {
                            throw std::invalid_argument("Function '" + identifier +
                                                        "' must be followed by parentheses.");
                        }
                        i = lookahead;
                        continue;
                    }

                    if (sawUnarySign && sign == -1)
                    {
                        tokens.push_back({Token::Type::Number, 0.0, 0, ""});
                        tokens.push_back({Token::Type::Operator, 0.0, '-', ""});
                    }
                    tokens.push_back({Token::Type::Variable, 0.0, 0, lowered});
                    expectValue = false;
                    continue;
                }

                throw std::invalid_argument("Expected a number or '(' in the expression.");
            }
            else
            {
                if (c == ')')
                {
                    tokens.push_back({Token::Type::RightParen, 0.0, 0, ""});
                    ++i;
                    continue;
                }

                if (c == '!')
                {
                    tokens.push_back({Token::Type::Operator, 0.0, '!', ""});
                    ++i;
                    continue;
                }

                if (isOperatorChar(c))
                {
                    tokens.push_back(
                        {Token::Type::Operator, 0.0, normalizeOperator(c), ""});
                    ++i;
                    expectValue = true;
                    continue;
                }

                throw std::invalid_argument("Expected an operator or ')' in the expression.");
            }
        }

        if (expectValue)
        {
            throw std::invalid_argument("Expression ended unexpectedly. Operand missing.");
        }

        return tokens;
    }

    int precedence(char op)
    {
        if (op == '+' || op == '-')
        {
            return 1;
        }
        if (op == '*' || op == '/')
        {
            return 2;
        }
        if (op == '!')
        {
            return 3;
        }
        throw std::invalid_argument("Unknown operator encountered.");
    }

    std::vector<Token> toRpn(const std::vector<Token> &tokens)
    {
        std::vector<Token> output;
        std::vector<Token> stack;

        for (const Token &token : tokens)
        {
            switch (token.type)
            {
            case Token::Type::Number:
                output.push_back(token);
                break;
            case Token::Type::Variable:
                output.push_back(token);
                break;
            case Token::Type::Function:
                stack.push_back(token);
                break;
            case Token::Type::Operator:
                while (!stack.empty() && stack.back().type == Token::Type::Operator &&
                       precedence(stack.back().op) >= precedence(token.op))
                {
                    output.push_back(stack.back());
                    stack.pop_back();
                }
                stack.push_back(token);
                break;
            case Token::Type::LeftParen:
                stack.push_back(token);
                break;
            case Token::Type::RightParen:
                while (!stack.empty() && stack.back().type != Token::Type::LeftParen)
                {
                    output.push_back(stack.back());
                    stack.pop_back();
                }
                if (stack.empty() || stack.back().type != Token::Type::LeftParen)
                {
                    throw std::invalid_argument("Mismatched parentheses in expression.");
                }
                stack.pop_back();
                if (!stack.empty() && stack.back().type == Token::Type::Function)
                {
                    output.push_back(stack.back());
                    stack.pop_back();
                }
                break;
            }
        }

        while (!stack.empty())
        {
            if (stack.back().type == Token::Type::LeftParen ||
                stack.back().type == Token::Type::RightParen)
            {
                throw std::invalid_argument("Mismatched parentheses in expression.");
            }
            output.push_back(stack.back());
            stack.pop_back();
        }

        return output;
    }
} // namespace

double evaluateExpression(const std::string &expression,
                          const std::map<std::string, double> &variables)
{
    std::vector<Token> tokens = tokenizeExpression(expression);
    std::vector<Token> rpn = toRpn(tokens);
    std::vector<double> stack;

    for (const Token &token : rpn)
    {
        switch (token.type)
        {
        case Token::Type::Number:
            stack.push_back(token.value);
            break;
        case Token::Type::Operator:
            if (token.op == '!')
            {
                if (stack.empty())
                {
                    throw std::invalid_argument("Factorial operator missing operand.");
                }
                double value = stack.back();
                stack.back() = factorialOf(value);
                break;
            }
            if (stack.size() < 2)
            {
                throw std::invalid_argument("Invalid expression: insufficient operands.");
            }
            {
                double rhs = stack.back();
                stack.pop_back();
                double lhs = stack.back();
                stack.pop_back();
                double result = 0.0;
                switch (token.op)
                {
                case '+':
                    result = lhs + rhs;
                    break;
                case '-':
                    result = lhs - rhs;
                    break;
                case '*':
                    result = lhs * rhs;
                    break;
                case '/':
                    if (rhs == 0.0)
                    {
                        throw std::runtime_error("Division by zero in expression.");
                    }
                    result = lhs / rhs;
                    break;
                default:
                    throw std::invalid_argument("Unknown operator in expression.");
                }
                stack.push_back(result);
            }
            break;
        case Token::Type::Function:
            if (stack.empty())
            {
                throw std::invalid_argument("Function missing operand.");
            }
            {
                double argument = stack.back();
                stack.back() = applyFunction(token.text, argument);
            }
            break;
        case Token::Type::Variable:
        {
            auto found = variables.find(token.text);
            if (found == variables.end())
            {
                throw std::invalid_argument("Unknown variable: " + token.text);
            }
            stack.push_back(found->second);
            break;
        }
        default:
            break;
        }
    }

    if (stack.size() != 1)
    {
        throw std::invalid_argument("Invalid expression: leftover operands.");
    }

    return stack.back();
}
