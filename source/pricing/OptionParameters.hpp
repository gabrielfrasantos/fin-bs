#pragma once

#include "source/core/Money.hpp"
#include "source/core/OptionContract.hpp"
#include "source/core/Rate.hpp"

namespace fin::pricing
{
    struct OptionParameters
    {
        core::Money spot;
        core::Money strike;
        core::Rate riskFreeRate;
        core::Rate dividendYield;
        double timeToExpiry;
        core::OptionType type;
    };
}
