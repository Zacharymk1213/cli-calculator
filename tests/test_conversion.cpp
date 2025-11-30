#include <gtest/gtest.h>
#include <stdexcept>

#include "core/numeral_conversion.hpp"

namespace
{
    std::string convert(const std::string &value, int fromBase, int toBase)
    {
        long long parsed = parseInteger(value, fromBase);
        return formatInteger(parsed, toBase);
    }
}

TEST(ConversionTest, DecimalToBinary)
{
    EXPECT_EQ(convert("10", 10, 2), "0b1010");
}

TEST(ConversionTest, BinaryToHex)
{
    EXPECT_EQ(convert("0b1111", 2, 16), "0xF");
}

TEST(ConversionTest, HexToDecimal)
{
    EXPECT_EQ(convert("0xFF", 16, 10), "255");
}

TEST(ConversionTest, NegativeNumbers)
{
    EXPECT_EQ(convert("-0x1A", 16, 2), "-0b11010");
}

TEST(ConversionTest, InvalidDigitThrows)
{
    EXPECT_THROW(parseInteger("10Z", 10), std::invalid_argument);
}
