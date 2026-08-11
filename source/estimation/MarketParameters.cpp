#include "source/estimation/MarketParameters.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/estimation/DriftEstimator.hpp"
#include "source/estimation/HistoricalVolatility.hpp"

namespace fin::estimation
{
    std::optional<MarketParameters> MarketParameters::From(const PriceSeries& series, const core::Rate& dividendYield, const LineFit& fit)
    {
        really_assert(dividendYield.Continuous() >= 0.0);

        const auto logDrift = DriftEstimator(fit).LogDrift(series);

        if (!logDrift)
            return std::nullopt;

        const auto volatility = HistoricalVolatility::From(series.LogReturns(), series.PeriodsPerYear());

        return MarketParameters(core::Rate::FromContinuous(logDrift->Continuous() + 0.5 * volatility.Variance()), volatility, dividendYield);
    }

    const core::Rate& MarketParameters::Drift() const
    {
        return drift;
    }

    const core::Volatility& MarketParameters::Volatility() const
    {
        return volatility;
    }

    const core::Rate& MarketParameters::DividendYield() const
    {
        return dividendYield;
    }

    MarketParameters::MarketParameters(const core::Rate& drift, const core::Volatility& volatility, const core::Rate& dividendYield)
        : drift(drift)
        , volatility(volatility)
        , dividendYield(dividendYield)
    {}
}
