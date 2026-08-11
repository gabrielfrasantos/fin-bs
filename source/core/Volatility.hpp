#pragma once

#include "infra/util/ReallyAssert.hpp"
#include <cmath>
#include <compare>

namespace fin::core
{
    class Volatility
    {
    public:
        static constexpr Volatility FromAnnualized(double annualized)
        {
            really_assert(annualized >= 0.0);
            return Volatility(annualized);
        }

        static Volatility FromVariance(double annualVariance)
        {
            really_assert(annualVariance >= 0.0);
            return Volatility(std::sqrt(annualVariance));
        }

        static Volatility FromPeriodDeviation(double periodDeviation, double periodsPerYear)
        {
            really_assert(periodDeviation >= 0.0);
            really_assert(periodsPerYear > 0.0);
            return Volatility(periodDeviation * std::sqrt(periodsPerYear));
        }

        static constexpr Volatility Zero()
        {
            return Volatility(0.0);
        }

        constexpr double Annualized() const
        {
            return annualized;
        }

        constexpr double Variance() const
        {
            return annualized * annualized;
        }

        double OverHorizon(double yearFraction) const
        {
            really_assert(yearFraction >= 0.0);
            return annualized * std::sqrt(yearFraction);
        }

        constexpr bool operator==(const Volatility& other) const = default;
        constexpr std::partial_ordering operator<=>(const Volatility& other) const = default;

    private:
        constexpr explicit Volatility(double annualized)
            : annualized(annualized)
        {}

        double annualized;
    };
}
