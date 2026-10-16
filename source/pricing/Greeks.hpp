#pragma once

#include "source/core/Delta.hpp"
#include "source/pricing/BlackScholes.hpp"

namespace fin::pricing
{
    class Greeks
    {
    public:
        Greeks(const OptionParameters& parameters, const core::Volatility& volatility);

        core::Delta Delta() const;
        double Gamma() const;
        double Vega() const;
        double Theta() const;
        double Rho() const;

    private:
        OptionParameters parameters;
        core::Volatility volatility;
        BlackScholes model;
    };
}
