#include "source/present/PriceHistory.hpp"
#include "source/core/DayCount.hpp"
#include "source/core/Rate.hpp"
#include "source/estimation/MarketParameters.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "source/io/PriceCsvLoader.hpp"
#include <cmath>

namespace fin::present
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;

        std::int32_t BasisPointsOf(double fraction)
        {
            return static_cast<std::int32_t>(std::llround(fraction * basisPointsPerUnit));
        }
    }

    PriceHistory::PriceHistory(const std::vector<std::string>& document, core::Currency currency, std::int32_t dividendYieldBasisPoints)
    {
        const io::PriceCsvLoader loader(document, currency, core::DayCountConvention::Act252);

        if (!loader.Series().has_value())
        {
            failure = loader.Failure();
            return;
        }

        const estimation::OrdinaryLeastSquaresFit fit;
        const auto yield = core::Rate::FromContinuous(static_cast<double>(dividendYieldBasisPoints) / basisPointsPerUnit);
        const auto parameters = estimation::MarketParameters::From(*loader.Series(), yield, fit);

        if (!parameters.has_value())
        {
            failure = "The price history does not hold enough closes to estimate from";
            return;
        }

        const auto volatility = BasisPointsOf(parameters->Volatility().Annualized());

        if (volatility < 1)
        {
            failure = "The price history does not move enough to estimate a volatility from";
            return;
        }

        estimate = PriceEstimate{ volatility, BasisPointsOf(parameters->Drift().Continuous()) };
    }

    const std::optional<PriceEstimate>& PriceHistory::Estimate() const
    {
        return estimate;
    }

    const std::string& PriceHistory::Failure() const
    {
        return failure;
    }
}
