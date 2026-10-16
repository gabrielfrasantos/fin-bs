#pragma once

#include "source/core/Volatility.hpp"
#include "source/pricing/OptionParameters.hpp"

namespace fin::pricing
{
    class BlackScholes
    {
    public:
        BlackScholes(const OptionParameters& parameters, const core::Volatility& volatility);

        double D1() const;
        double D2() const;
        double Deviation() const;
        double SpotDiscountFactor() const;
        double StrikeDiscountFactor() const;
        double Price() const;
        double IntrinsicValue() const;
        double ExtrinsicValue() const;

    private:
        double spot;
        double strike;
        double spotDiscountFactor;
        double strikeDiscountFactor;
        double deviation;
        core::OptionType type;
        double d1{};
        double d2{};
    };
}
