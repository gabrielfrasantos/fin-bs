#include "source/sim/Calibration.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/estimation/DriftEstimator.hpp"
#include "source/estimation/EmaVolatilityEstimator.hpp"
#include "source/estimation/HistoricalVolatility.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace fin::sim
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;
        constexpr double leastVolatility = 0.0001;

        std::string Padded(std::uint32_t part)
        {
            return part < 10 ? "0" + std::to_string(part) : std::to_string(part);
        }

        std::string TextOf(const core::Date& date)
        {
            return std::to_string(date.Year()) + "-" + Padded(date.Month()) + "-" + Padded(date.Day());
        }

        estimation::PriceSeries WindowBefore(const estimation::PriceSeries& history, std::size_t end, std::size_t observations)
        {
            const auto dates = history.Dates().subspan(end - observations, observations);
            const auto closes = history.Closes().subspan(end - observations, observations);

            return estimation::PriceSeries::From(std::vector<core::Date>(dates.begin(), dates.end()), std::vector<core::Money>(closes.begin(), closes.end()), history.Convention());
        }

        core::Volatility VolatilityOf(const CalibrationRule& rule, const estimation::PriceSeries& window)
        {
            const auto logReturns = window.LogReturns();

            if (rule.volatility == VolatilityEstimator::exponentiallyWeighted)
                return estimation::EmaVolatilityEstimator::WithHalfLife(static_cast<double>(rule.halfLifeObservations), window.PeriodsPerYear()).From(logReturns);

            return estimation::HistoricalVolatility::From(logReturns, window.PeriodsPerYear());
        }

        std::optional<core::Rate> DriftOf(const CalibrationRule& rule, const estimation::PriceSeries& window, const core::Volatility& volatility, const UnderlyingTerms& stated)
        {
            if (rule.drift == DriftEstimator::stated)
                return stated.drift;

            const estimation::OrdinaryLeastSquaresFit fit;
            const auto logDrift = estimation::DriftEstimator(fit).LogDrift(window);

            if (!logDrift.has_value())
                return std::nullopt;

            return core::Rate::FromContinuous(logDrift->Continuous() + 0.5 * volatility.Variance());
        }
    }

    Calibrator::Calibrator(const CalibrationRule& rule, const estimation::PriceSeries& history, const core::Date& on, const UnderlyingTerms& stated)
    {
        really_assert(IsWellFormed(rule));

        const auto dates = history.Dates();
        const auto end = static_cast<std::size_t>(std::lower_bound(dates.begin(), dates.end(), on) - dates.begin());
        const auto observations = static_cast<std::size_t>(rule.windowObservations);

        if (end < observations)
        {
            failure = "The history holds " + std::to_string(end) + " closes before " + TextOf(on) + ", and the rule needs " + std::to_string(rule.windowObservations);
            return;
        }

        const auto window = WindowBefore(history, end, observations);
        const auto volatility = VolatilityOf(rule, window);

        if (volatility.Annualized() < leastVolatility)
        {
            failure = "The closes before " + TextOf(on) + " do not move enough to estimate a volatility from";
            return;
        }

        const auto drift = DriftOf(rule, window, volatility, stated);

        if (!drift.has_value())
        {
            failure = "The closes before " + TextOf(on) + " span no time to estimate a drift over";
            return;
        }

        const auto implied = volatility.Annualized() + static_cast<double>(rule.impliedPremiumBasisPoints) / basisPointsPerUnit;

        if (implied < leastVolatility)
        {
            failure = "The implied premium takes the implied volatility before " + TextOf(on) + " to nothing";
            return;
        }

        estimate = CalibratedEstimate{ .windowEnd = dates[end - 1],
            .observations = rule.windowObservations,
            .drift = *drift,
            .volatility = volatility,
            .impliedVolatility = core::Volatility::FromAnnualized(implied) };
    }

    const std::optional<CalibratedEstimate>& Calibrator::Estimate() const
    {
        return estimate;
    }

    const std::string& Calibrator::Failure() const
    {
        return failure;
    }

    bool IsWellFormed(const CalibrationRule& rule)
    {
        if (rule.windowObservations < leastWindowObservations || rule.windowObservations > mostWindowObservations)
            return false;

        if (rule.halfLifeObservations < 1 || rule.halfLifeObservations > rule.windowObservations)
            return false;

        return rule.impliedPremiumBasisPoints >= -basisPointsPerUnit && rule.impliedPremiumBasisPoints <= basisPointsPerUnit;
    }

    UnderlyingTerms Calibrated(const UnderlyingTerms& stated, const CalibratedEstimate& estimate)
    {
        auto terms = stated;
        terms.drift = estimate.drift;
        terms.volatility = estimate.volatility;
        terms.impliedVolatility = estimate.impliedVolatility;

        return terms;
    }
}
