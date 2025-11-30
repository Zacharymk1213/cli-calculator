#include "matrix.hpp"

#include <stdexcept>

namespace
{
    void validateRectangular(const Matrix &matrix)
    {
        if (matrix.empty())
        {
            throw std::invalid_argument("Matrix must contain at least one row.");
        }
        std::size_t columnCount = matrix.front().size();
        if (columnCount == 0)
        {
            throw std::invalid_argument("Matrix rows must contain at least one element.");
        }
        for (const auto &row : matrix)
        {
            if (row.size() != columnCount)
            {
                throw std::invalid_argument("All matrix rows must contain the same number of elements.");
            }
        }
    }

    void validateSameSize(const Matrix &lhs, const Matrix &rhs)
    {
        validateRectangular(lhs);
        validateRectangular(rhs);
        if (lhs.size() != rhs.size() || lhs.front().size() != rhs.front().size())
        {
            throw std::invalid_argument("Matrices must have equal dimensions.");
        }
    }
} // namespace

Matrix addMatrices(const Matrix &lhs, const Matrix &rhs)
{
    validateSameSize(lhs, rhs);
    Matrix result(lhs.size(), std::vector<double>(lhs.front().size(), 0.0));
    for (std::size_t row = 0; row < lhs.size(); ++row)
    {
        for (std::size_t col = 0; col < lhs[row].size(); ++col)
        {
            result[row][col] = lhs[row][col] + rhs[row][col];
        }
    }
    return result;
}

Matrix subtractMatrices(const Matrix &lhs, const Matrix &rhs)
{
    validateSameSize(lhs, rhs);
    Matrix result(lhs.size(), std::vector<double>(lhs.front().size(), 0.0));
    for (std::size_t row = 0; row < lhs.size(); ++row)
    {
        for (std::size_t col = 0; col < lhs[row].size(); ++col)
        {
            result[row][col] = lhs[row][col] - rhs[row][col];
        }
    }
    return result;
}

Matrix multiplyMatrices(const Matrix &lhs, const Matrix &rhs)
{
    validateRectangular(lhs);
    validateRectangular(rhs);
    if (lhs.front().size() != rhs.size())
    {
        throw std::invalid_argument("Left matrix column count must equal right matrix row count.");
    }

    std::size_t rows = lhs.size();
    std::size_t columns = rhs.front().size();
    std::size_t shared = lhs.front().size();

    Matrix result(rows, std::vector<double>(columns, 0.0));
    for (std::size_t row = 0; row < rows; ++row)
    {
        for (std::size_t col = 0; col < columns; ++col)
        {
            double sum = 0.0;
            for (std::size_t idx = 0; idx < shared; ++idx)
            {
                sum += lhs[row][idx] * rhs[idx][col];
            }
            result[row][col] = sum;
        }
    }
    return result;
}
