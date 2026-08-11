#pragma once

#include "source/core/Volatility.hpp"
#include "source/pricing/OptionParameters.hpp"
#include <optional>

namespace fin::pricing
{
    class ImpliedVolatility
    {
    public:
        static std::optional<core::Volatility> From(const OptionParameters& parameters, double observedPrice);
    };
}
