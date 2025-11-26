#include "equations.hpp"

#include "ansi_colors.hpp"
#include "math_utils.hpp"

#include <complex>
#include <iostream>

void solveLinearEquation(double a, double b)
{
    if (isApproximatelyZero(a))
    {
        if (isApproximatelyZero(b))
        {
            std::cout << CYAN << "Every real number is a solution." << RESET << '\n';
        }
        else
        {
            std::cout << RED << "No solution exists for this equation." << RESET << '\n';
        }
        return;
    }

    double result = -b / a;
    std::cout << GREEN << "Solution: x = " << RESET << result << '\n';
}

void solveQuadraticEquation(double a, double b, double c)
{
    if (isApproximatelyZero(a))
    {
        std::cout << YELLOW << "Coefficient 'a' is zero; falling back to a linear equation." << RESET << '\n';
        solveLinearEquation(b, c);
        return;
    }

    double discriminant = b * b - 4.0 * a * c;
    constexpr double epsilon = 1e-9;

    if (discriminant > epsilon)
    {
        double sqrtDisc = std::sqrt(discriminant);
        double denom = 2.0 * a;
        std::cout << CYAN << "Two real solutions:" << RESET << '\n';
        std::cout << GREEN << " x1 = " << RESET << (-b + sqrtDisc) / denom << '\n';
        std::cout << GREEN << " x2 = " << RESET << (-b - sqrtDisc) / denom << '\n';
    }
    else if (isApproximatelyZero(discriminant, epsilon))
    {
        double root = -b / (2.0 * a);
        std::cout << GREEN << "One real solution (double root): x = " << RESET << root << '\n';
    }
    else
    {
        std::complex<double> sqrtDisc = std::sqrt(std::complex<double>(discriminant, 0.0));
        std::complex<double> denom(2.0 * a, 0.0);
        std::complex<double> x1 = (-b + sqrtDisc) / denom;
        std::complex<double> x2 = (-b - sqrtDisc) / denom;

        auto printComplex = [](const std::complex<double> &value)
        {
            double realPart = value.real();
            double imagPart = value.imag();
            std::cout << GREEN << realPart << RESET;
            if (!isApproximatelyZero(imagPart))
            {
                if (imagPart >= 0)
                {
                    std::cout << " + " << GREEN << imagPart << RESET << "i";
                }
                else
                {
                    std::cout << " - " << GREEN << std::abs(imagPart) << RESET << "i";
                }
            }
        };

        std::cout << "Two complex solutions:\n x1 = ";
        printComplex(x1);
        std::cout << "\n x2 = ";
        printComplex(x2);
        std::cout << '\n';
    }
}
