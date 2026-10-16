#pragma once

#include "infra/util/ReallyAssert.hpp"
#include <compare>

namespace fin::core
{
    class Delta
    {
    public:
        static constexpr Delta FromValue(double value)
        {
            really_assert(value >= -1.0 && value <= 1.0);
            return Delta(value);
        }

        constexpr double Value() const
        {
            return value;
        }

        constexpr double Magnitude() const
        {
            return value < 0.0 ? -value : value;
        }

        constexpr Delta operator-() const
        {
            return Delta(-value);
        }

        constexpr bool operator==(const Delta& other) const = default;
        constexpr std::partial_ordering operator<=>(const Delta& other) const = default;

    private:
        constexpr explicit Delta(double value)
            : value(value)
        {}

        double value;
    };
}
