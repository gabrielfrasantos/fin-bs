#include "source/estimation/DriftEstimator.hpp"
#include <algorithm>
#include <functional>

namespace fin::estimation
{
    namespace
    {
        bool NeverMoves(std::span<const core::Money> closes)
        {
            return std::adjacent_find(closes.begin(), closes.end(), std::not_equal_to<>{}) == closes.end();
        }
    }

    DriftEstimator::DriftEstimator(const LineFit& fit)
        : fit(fit)
    {}

    std::optional<core::Rate> DriftEstimator::LogDrift(const PriceSeries& series) const
    {
        if (NeverMoves(series.Closes()))
            return core::Rate::Zero();

        const auto slope = fit.Slope(series.Times(), series.LogPrices());

        if (!slope)
            return std::nullopt;

        return core::Rate::FromContinuous(*slope);
    }
}
