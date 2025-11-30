#include <gtest/gtest.h>
#include <stdexcept>

#include "core/expression.hpp"

TEST(ErrorTest, InvalidSyntax)
{
    EXPECT_THROW(evaluateExpression("2+*3"), std::invalid_argument);
}

TEST(ErrorTest, DivisionByZero)
{
    EXPECT_THROW(evaluateExpression("5 / 0"), std::runtime_error);
}

TEST(ErrorTest, UnknownFunction)
{
    EXPECT_THROW(evaluateExpression("foo(10)"), std::invalid_argument);
}

TEST(ErrorTest, EmptyInput)
{
    EXPECT_THROW(evaluateExpression(""), std::invalid_argument);
}
