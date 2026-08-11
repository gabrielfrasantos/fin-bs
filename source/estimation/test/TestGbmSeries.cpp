#include "source/estimation/HistoricalVolatility.hpp"
#include "source/estimation/PriceSeries.hpp"
#include "source/estimation/test_doubles/GbmSeries.hpp"
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <span>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::estimation::HistoricalVolatility;
    using fin::estimation::PriceSeries;
    using fin::estimation::test_doubles::GbmSeries;

    class TestGbmSeries
        : public ::testing::Test
    {
    protected:
        GbmSeries::Parameters Parameters(double drift, double volatility) const
        {
            return GbmSeries::Parameters{
                .start = Date::FromYearMonthDay(2000, 1, 1),
                .initialPrice = Money::FromAmount(100.0, Currency::Eur),
                .drift = Rate::FromContinuous(drift),
                .volatility = Volatility::FromAnnualized(volatility)
            };
        }

        PriceSeries Series(double drift, double volatility, uint64_t seed, std::size_t observations) const
        {
            GbmSeries generator(Parameters(drift, volatility), seed);
            return generator.Generate(observations);
        }
    };
}

TEST_F(TestGbmSeries, a_generated_series_starts_at_the_initial_price_on_a_business_day)
{
    const auto series = Series(0.07, 0.2, 1u, 50);

    EXPECT_EQ(series.Size(), 50u);
    EXPECT_EQ(series.Closes().front(), Money::FromAmount(100.0, Currency::Eur));
    EXPECT_FALSE(series.Dates().front().IsWeekend());
    EXPECT_EQ(series.Convention(), DayCountConvention::Act252);
}

TEST_F(TestGbmSeries, every_observation_falls_on_a_business_day_one_trading_day_after_the_previous)
{
    const auto series = Series(0.07, 0.2, 2u, 400);
    const auto times = series.Times();

    for (const auto& date : series.Dates())
        EXPECT_FALSE(date.IsWeekend());

    for (std::size_t i = 0; i != times.size(); ++i)
        EXPECT_DOUBLE_EQ(times[i], static_cast<double>(i) / GbmSeries::periodsPerYear);

    EXPECT_DOUBLE_EQ(series.PeriodsPerYear(), GbmSeries::periodsPerYear);
}

TEST_F(TestGbmSeries, the_same_seed_produces_the_same_series)
{
    const auto first = Series(0.07, 0.2, 12345u, 200);
    const auto second = Series(0.07, 0.2, 12345u, 200);

    EXPECT_TRUE(std::equal(first.Closes().begin(), first.Closes().end(), second.Closes().begin()));
}

TEST_F(TestGbmSeries, a_different_seed_produces_a_different_series)
{
    const auto first = Series(0.07, 0.2, 12345u, 200);
    const auto second = Series(0.07, 0.2, 54321u, 200);

    EXPECT_FALSE(std::equal(first.Closes().begin(), first.Closes().end(), second.Closes().begin()));
}

TEST_F(TestGbmSeries, without_volatility_the_path_is_the_deterministic_growth_of_the_drift)
{
    const auto series = Series(0.10, 0.0, 7u, 300);
    const auto times = series.Times();
    const auto closes = series.Closes();

    for (std::size_t i = 0; i != closes.size(); ++i)
        EXPECT_NEAR(closes[i].ToDouble(), 100.0 * std::exp(0.10 * times[i]), 0.01);
}

TEST_F(TestGbmSeries, the_realised_volatility_of_a_long_path_is_the_volatility_it_was_given)
{
    const auto series = Series(0.0, 0.25, 99u, 20000);

    EXPECT_NEAR(HistoricalVolatility::From(series.LogReturns(), series.PeriodsPerYear()).Annualized(), 0.25, 0.01);
}

TEST_F(TestGbmSeries, the_realised_log_growth_of_a_long_path_is_the_drift_less_half_the_variance)
{
    const auto series = Series(0.08, 0.05, 4u, 20000);
    const auto closes = series.Closes();
    const auto realised = std::log(closes.back().ToDouble() / closes.front().ToDouble()) / series.Horizon();

    EXPECT_NEAR(realised, 0.08 - 0.5 * 0.05 * 0.05, 0.01);
}

TEST_F(TestGbmSeries, a_regime_switch_raises_the_volatility_of_the_second_half_only)
{
    GbmSeries generator(Parameters(0.0, 0.10), 31u);
    const auto series = generator.GenerateWithRegimeSwitch(20000, Volatility::FromAnnualized(0.40));
    const auto returns = series.LogReturns();

    const std::span<const double> first{ returns.data(), returns.size() / 2 };
    const std::span<const double> second{ returns.data() + returns.size() / 2, returns.size() / 2 };

    EXPECT_NEAR(HistoricalVolatility::From(first, series.PeriodsPerYear()).Annualized(), 0.10, 0.01);
    EXPECT_NEAR(HistoricalVolatility::From(second, series.PeriodsPerYear()).Annualized(), 0.40, 0.02);
}
