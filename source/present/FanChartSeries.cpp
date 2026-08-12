#include "source/present/FanChartSeries.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <cmath>

namespace fin::present
{
    namespace
    {
        constexpr std::int32_t lowerBand = 5;
        constexpr std::int32_t median = 50;
        constexpr std::int32_t upperBand = 95;
        constexpr std::int32_t wholeDistribution = 100;
        constexpr std::size_t ticksPerAxis = 5;
        constexpr double leastSpread = 1.0;
        constexpr double relativeSpread = 0.01;

        double Elapsed(const core::Date& from, const core::Date& until)
        {
            return static_cast<double>(until - from);
        }

        core::Money RankedAt(std::vector<core::Money>& equity, std::int32_t percentile)
        {
            const auto paths = static_cast<std::int64_t>(equity.size());
            const auto rank = (percentile * paths + wholeDistribution - 1) / wholeDistribution;
            const auto position = equity.begin() + static_cast<std::ptrdiff_t>(rank - 1);

            std::nth_element(equity.begin(), position, equity.end());

            return *position;
        }

        ChartAxis SpanningFrom(double lowest, double highest)
        {
            if (!(highest > lowest))
            {
                const auto spread = std::max(leastSpread, std::abs(lowest) * relativeSpread);
                lowest -= spread;
                highest += spread;
            }

            ChartAxis axis{ lowest, highest, {} };
            axis.ticks.reserve(ticksPerAxis);

            for (std::size_t tick = 0; tick != ticksPerAxis; ++tick)
                axis.ticks.push_back(lowest + (highest - lowest) * static_cast<double>(tick) / static_cast<double>(ticksPerAxis - 1));

            return axis;
        }

        ChartAxis Over(std::span<const core::Date> boundaries)
        {
            if (boundaries.empty())
                return SpanningFrom(0.0, 0.0);

            return SpanningFrom(0.0, Elapsed(boundaries.front(), boundaries.back()));
        }

        ChartAxis Across(const FanChartSeries& series)
        {
            if (series.median.empty())
                return SpanningFrom(0.0, 0.0);

            auto lowest = series.lower.front().value;
            auto highest = series.upper.front().value;

            for (const auto& point : series.lower)
                lowest = std::min(lowest, point.value);

            for (const auto& point : series.upper)
                highest = std::max(highest, point.value);

            return SpanningFrom(lowest, highest);
        }
    }

    FanChartSeries FanOf(const sim::OutcomeDistribution& distribution, std::span<const core::Date> boundaries)
    {
        const auto outcomes = distribution.Outcomes();

        for (const auto& outcome : outcomes)
            really_assert(outcome.equityByCycle.size() == boundaries.size());

        FanChartSeries series;

        for (std::size_t boundary = 0; boundary != boundaries.size(); ++boundary)
        {
            std::vector<core::Money> equity;
            equity.reserve(outcomes.size());

            for (const auto& outcome : outcomes)
                equity.push_back(outcome.equityByCycle[boundary]);

            const auto at = Elapsed(boundaries.front(), boundaries[boundary]);

            series.lower.push_back({ at, RankedAt(equity, lowerBand).ToDouble() });
            series.median.push_back({ at, RankedAt(equity, median).ToDouble() });
            series.upper.push_back({ at, RankedAt(equity, upperBand).ToDouble() });
        }

        series.horizontal = Over(boundaries);
        series.vertical = Across(series);

        return series;
    }

    FanChartSeries TraceOf(std::span<const core::Money> equity, std::span<const core::Date> boundaries)
    {
        really_assert(equity.size() == boundaries.size());

        FanChartSeries series;

        for (std::size_t boundary = 0; boundary != boundaries.size(); ++boundary)
        {
            const ChartPoint point{ Elapsed(boundaries.front(), boundaries[boundary]), equity[boundary].ToDouble() };

            series.lower.push_back(point);
            series.median.push_back(point);
            series.upper.push_back(point);
        }

        series.horizontal = Over(boundaries);
        series.vertical = Across(series);

        return series;
    }

    bool IsEmpty(const FanChartSeries& series)
    {
        return series.median.empty();
    }
}
