#pragma once

#include "source/pricing/OptionParameters.hpp"

namespace fin::pricing::test
{
    inline OptionParameters OptionScenario(double spot, double strike, double riskFreeRate, double dividendYield, double timeToExpiry, core::OptionType type)
    {
        return OptionParameters{
            .spot = core::Money::FromAmount(spot, core::Currency::Eur),
            .strike = core::Money::FromAmount(strike, core::Currency::Eur),
            .riskFreeRate = core::Rate::FromContinuous(riskFreeRate),
            .dividendYield = core::Rate::FromContinuous(dividendYield),
            .timeToExpiry = timeToExpiry,
            .type = type
        };
    }
}
