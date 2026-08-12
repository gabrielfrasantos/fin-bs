#pragma once

#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace fin::present
{
    struct ChartPoint
    {
        double at;
        double value;

        bool operator==(const ChartPoint& other) const = default;
    };

    struct ChartAxis
    {
        double lowest;
        double highest;
        std::vector<double> ticks;

        bool operator==(const ChartAxis& other) const = default;
    };

    struct FanChartSeries
    {
        std::vector<ChartPoint> lower;
        std::vector<ChartPoint> median;
        std::vector<ChartPoint> upper;
        ChartAxis horizontal;
        ChartAxis vertical;

        bool operator==(const FanChartSeries& other) const = default;
    };

    FanChartSeries FanOf(const sim::OutcomeDistribution& distribution, std::span<const core::Date> boundaries);
    FanChartSeries TraceOf(std::span<const core::Money> equity, std::span<const core::Date> boundaries);
    bool IsEmpty(const FanChartSeries& series);
}
