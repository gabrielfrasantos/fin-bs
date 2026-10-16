#include "source/present/FanChartSeries.hpp"
#include "source/present/test/RunScenario.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Date;
    using fin::core::Money;
    using fin::present::FanOf;
    using fin::present::IsEmpty;
    using fin::present::TraceOf;
    using fin::present::test::Euro;
    using fin::sim::OutcomeDistribution;
    using fin::sim::PathOutcome;
    using ::testing::ElementsAre;

    std::vector<Date> Boundaries()
    {
        return { Date::FromYearMonthDay(2026, 1, 5), Date::FromYearMonthDay(2026, 7, 6), Date::FromYearMonthDay(2027, 1, 4) };
    }

    PathOutcome Walked(double terminal, double middle)
    {
        return PathOutcome{ .terminalEquity = Euro(terminal),
            .growthRate = 0.0,
            .positions = 4,
            .assignments = 0,
            .maximumDrawdown = 0.0,
            .equityByCycle = { Euro(200000.00), Euro(middle), Euro(terminal) } };
    }

    OutcomeDistribution Spread()
    {
        return OutcomeDistribution::Of({ Walked(240000.00, 190000.00),
            Walked(220000.00, 230000.00),
            Walked(180000.00, 210000.00),
            Walked(160000.00, 205000.00) });
    }

    TEST(TestFanChartSeries, places_a_point_on_every_cycle_boundary)
    {
        const auto series = FanOf(Spread(), Boundaries());

        EXPECT_FALSE(IsEmpty(series));
        EXPECT_EQ(series.median.size(), 3u);
        EXPECT_EQ(series.median.front().at, 0.0);
        EXPECT_EQ(series.median.back().at, 364.0);
    }

    TEST(TestFanChartSeries, holds_the_bands_in_order_at_every_date)
    {
        const auto series = FanOf(Spread(), Boundaries());

        for (std::size_t point = 0; point != series.median.size(); ++point)
        {
            EXPECT_LE(series.lower[point].value, series.median[point].value);
            EXPECT_LE(series.median[point].value, series.upper[point].value);
        }
    }

    TEST(TestFanChartSeries, ranks_every_date_on_its_own_rather_than_on_the_path_that_ends_highest)
    {
        const auto series = FanOf(Spread(), Boundaries());

        EXPECT_EQ(series.lower[1].value, 190000.00);
        EXPECT_EQ(series.upper[1].value, 230000.00);
        EXPECT_EQ(series.lower[2].value, 160000.00);
        EXPECT_EQ(series.upper[2].value, 240000.00);
    }

    TEST(TestFanChartSeries, spans_an_axis_that_a_flat_run_can_still_be_drawn_on)
    {
        const std::vector<Money> flat{ Euro(200000.00), Euro(200000.00), Euro(200000.00) };
        const auto series = TraceOf(flat, Boundaries());

        EXPECT_LT(series.vertical.lowest, series.vertical.highest);
        EXPECT_LT(series.horizontal.lowest, series.horizontal.highest);
        EXPECT_EQ(series.vertical.ticks.size(), 5u);
        EXPECT_EQ(series.vertical.ticks.front(), series.vertical.lowest);
        EXPECT_EQ(series.vertical.ticks.back(), series.vertical.highest);
    }

    TEST(TestFanChartSeries, draws_one_line_three_times_over_when_a_single_run_is_traced)
    {
        const std::vector<Money> walked{ Euro(200000.00), Euro(205000.00), Euro(215000.00) };
        const auto series = TraceOf(walked, Boundaries());

        EXPECT_EQ(series.lower, series.median);
        EXPECT_EQ(series.upper, series.median);
        EXPECT_THAT(series.median, ElementsAre(fin::present::ChartPoint{ 0.0, 200000.00 }, fin::present::ChartPoint{ 182.0, 205000.00 }, fin::present::ChartPoint{ 364.0, 215000.00 }));
    }

    TEST(TestFanChartSeries, paints_nothing_when_nothing_was_run)
    {
        const auto series = TraceOf({}, {});

        EXPECT_TRUE(IsEmpty(series));
        EXPECT_LT(series.vertical.lowest, series.vertical.highest);
        EXPECT_LT(series.horizontal.lowest, series.horizontal.highest);
    }
}
