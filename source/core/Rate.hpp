#pragma once

#include "infra/util/ReallyAssert.hpp"
#include <cmath>
#include <compare>

namespace fin::core
{
    class Rate
    {
    public:
        static constexpr Rate FromContinuous(double continuouslyCompounded)
        {
            return Rate(continuouslyCompounded);
        }

        static Rate FromEffectiveAnnual(double effectiveAnnual)
        {
            really_assert(effectiveAnnual > -1.0);
            return Rate(std::log1p(effectiveAnnual));
        }

        static constexpr Rate Zero()
        {
            return Rate(0.0);
        }

        constexpr double Continuous() const
        {
            return continuous;
        }

        double EffectiveAnnual() const
        {
            return std::expm1(continuous);
        }

        double GrowthFactor(double yearFraction) const
        {
            return std::exp(continuous * yearFraction);
        }

        double DiscountFactor(double yearFraction) const
        {
            return std::exp(-continuous * yearFraction);
        }

        constexpr bool operator==(const Rate& other) const = default;
        constexpr std::partial_ordering operator<=>(const Rate& other) const = default;

    private:
        constexpr explicit Rate(double continuous)
            : continuous(continuous)
        {}

        double continuous;
    };
}
