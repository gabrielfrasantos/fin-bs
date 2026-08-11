#pragma once

#include "source/core/Rate.hpp"
#include "source/core/Volatility.hpp"
#include "source/estimation/LineFit.hpp"
#include "source/estimation/PriceSeries.hpp"
#include <optional>

namespace fin::estimation
{
    class MarketParameters
    {
    public:
        static std::optional<MarketParameters> From(const PriceSeries& series, const core::Rate& dividendYield, const LineFit& fit);

        const core::Rate& Drift() const;
        const core::Volatility& Volatility() const;
        const core::Rate& DividendYield() const;

    private:
        MarketParameters(const core::Rate& drift, const core::Volatility& volatility, const core::Rate& dividendYield);

        core::Rate drift;
        core::Volatility volatility;
        core::Rate dividendYield;
    };
}
