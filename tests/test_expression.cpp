#include <gtest/gtest.h>

#include "core/expression.hpp"

TEST(ExpressionTest, SimpleArithmetic)
{
    EXPECT_DOUBLE_EQ(evaluateExpression("1+1"), 2);
    EXPECT_DOUBLE_EQ(evaluateExpression("10 - 3"), 7);
    EXPECT_DOUBLE_EQ(evaluateExpression("2 * 5"), 10);
    EXPECT_DOUBLE_EQ(evaluateExpression("20 / 4"), 5);
}

TEST(ExpressionTest, ComplexArithmetic)
{
    EXPECT_DOUBLE_EQ(evaluateExpression("2+3*4"), 14);
    EXPECT_DOUBLE_EQ(evaluateExpression("(2+3)*4"), 20);
    EXPECT_NEAR(evaluateExpression("3.5 * 2.2"), 7.7, 1e-9);
}

TEST(ExpressionTest, TrigonometryAndFunctions)
{
    EXPECT_NEAR(evaluateExpression("sin(0)"), 0.0, 1e-9);
    EXPECT_NEAR(evaluateExpression("cos(0)"), 1.0, 1e-9);
    EXPECT_NEAR(evaluateExpression("tan(0.78539816339)"), 1.0, 1e-6);
    EXPECT_NEAR(evaluateExpression("log(exp(1))"), 1.0, 1e-9);
}

TEST(ExpressionTest, Factorial)
{
    EXPECT_DOUBLE_EQ(evaluateExpression("5!"), 120);
}
