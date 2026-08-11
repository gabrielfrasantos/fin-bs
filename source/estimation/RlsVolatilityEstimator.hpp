#pragma once

#include "source/core/Volatility.hpp"
#include <span>
#include <vector>

namespace fin::estimation
{
    class RlsVolatilityEstimator
    {
    public:
        static RlsVolatilityEstimator WithForgettingFactor(double forgettingFactor, double periodsPerYear);

        double ForgettingFactor() const;
        core::Volatility From(std::span<const double> logReturns) const;
        std::vector<core::Volatility> Track(std::span<const double> logReturns) const;

    private:
        RlsVolatilityEstimator(double forgettingFactor, double periodsPerYear);

        double forgettingFactor;
        double periodsPerYear;
    };
}
