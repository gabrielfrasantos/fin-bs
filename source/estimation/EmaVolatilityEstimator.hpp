#pragma once

#include "source/core/Volatility.hpp"
#include <span>
#include <vector>

namespace fin::estimation
{
    class EmaVolatilityEstimator
    {
    public:
        static EmaVolatilityEstimator WithSmoothingFactor(double smoothingFactor, double periodsPerYear);
        static EmaVolatilityEstimator WithHalfLife(double halfLifeInPeriods, double periodsPerYear);

        double SmoothingFactor() const;
        core::Volatility From(std::span<const double> logReturns) const;
        std::vector<core::Volatility> Track(std::span<const double> logReturns) const;

    private:
        EmaVolatilityEstimator(double smoothingFactor, double periodsPerYear);

        double smoothingFactor;
        double periodsPerYear;
    };
}
