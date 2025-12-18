#pragma once
#include <cstddef>
#include <vector>

using Matrix = std::vector<std::vector<double>>;

Matrix addMatrices(const Matrix &lhs, const Matrix &rhs);
Matrix subtractMatrices(const Matrix &lhs, const Matrix &rhs);
Matrix multiplyMatrices(const Matrix &lhs, const Matrix &rhs);
