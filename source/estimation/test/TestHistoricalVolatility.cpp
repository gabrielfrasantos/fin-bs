#include "source/estimation/HistoricalVolatility.hpp"
#include "source/estimation/LogReturns.hpp"
#include "source/estimation/test_doubles/GbmSeries.hpp"
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::estimation::HistoricalVolatility;
    using fin::estimation::test_doubles::GbmSeries;

    std::vector<double> Alternating(std::size_t count, double magnitude)
    {
        std::vector<double> result;

        for (std::size_t i = 0; i != count; ++i)
            result.push_back(i % 2 == 0 ? magnitude : -magnitude);

        return result;
    }

    class TestHistoricalVolatility
        : public ::testing::Test
    {
    protected:
        GbmSeries::Parameters ParametersWith(double annualVolatility) const
        {
            return GbmSeries::Parameters{
                .start = Date::FromYearMonthDay(2000, 1, 3),
                .initialPrice = Money::FromAmount(100.0, Currency::Eur),
                .drift = Rate::FromContinuous(0.07),
                .volatility = Volatility::FromAnnualized(annualVolatility)
            };
        }

        static constexpr double tradingDaysPerYear = 252.0;
    };
}

TEST_F(TestHistoricalVolatility, a_series_that_never_moves_has_no_volatility)
{
    const std::vector<double> unchanged(100, 0.0);

    EXPECT_EQ(HistoricalVolatility::From(unchanged, tradingDaysPerYear), Volatility::Zero());
}

TEST_F(TestHistoricalVolatility, a_constant_magnitude_series_has_the_deviation_of_that_magnitude)
{
    const auto returns = Alternating(1000, 0.01);
    const auto expected = 0.01 * std::sqrt(1000.0 / 999.0) * std::sqrt(tradingDaysPerYear);

    EXPECT_NEAR(HistoricalVolatility::From(returns, tradingDaysPerYear).Annualized(), expected, 1e-12);
}

TEST_F(TestHistoricalVolatility, doubling_every_return_doubles_the_volatility)
{
    const auto single = HistoricalVolatility::From(Alternating(500, 0.004), tradingDaysPerYear);
    const auto doubled = HistoricalVolatility::From(Alternating(500, 0.008), tradingDaysPerYear);

    EXPECT_NEAR(doubled.Annualized(), 2.0 * single.Annualized(), 1e-12);
}

TEST_F(TestHistoricalVolatility, annualisation_scales_with_the_square_root_of_the_observation_frequency)
{
    const auto returns = Alternating(500, 0.01);
    const auto daily = HistoricalVolatility::From(returns, tradingDaysPerYear);
    const auto monthly = HistoricalVolatility::From(returns, 12.0);

    EXPECT_NEAR(daily.Annualized(), monthly.Annualized() * std::sqrt(tradingDaysPerYear / 12.0), 1e-12);
}

TEST_F(TestHistoricalVolatility, the_estimate_does_not_depend_on_the_order_of_the_returns)
{
    auto returns = Alternating(101, 0.013);
    const auto forward = HistoricalVolatility::From(returns, tradingDaysPerYear);

    std::reverse(returns.begin(), returns.end());

    EXPECT_NEAR(HistoricalVolatility::From(returns, tradingDaysPerYear).Annualized(), forward.Annualized(), 1e-15);
}

TEST_F(TestHistoricalVolatility, a_known_volatility_is_recovered_from_a_synthetic_price_series)
{
    for (auto annualVolatility : { 0.12, 0.20, 0.35 })
    {
        GbmSeries generator(ParametersWith(annualVolatility), 20240101u);
        const auto series = generator.Generate(5000);
        const auto estimated = HistoricalVolatility::From(series.LogReturns(), series.PeriodsPerYear());

        EXPECT_NEAR(estimated.Annualized(), annualVolatility, 0.05 * annualVolatility);
    }
}

TEST_F(TestHistoricalVolatility, the_documented_worked_example_is_reproduced)
{
    const std::vector<Money> closes{
        Money::FromAmount(100.00, Currency::Eur),
        Money::FromAmount(102.00, Currency::Eur),
        Money::FromAmount(101.00, Currency::Eur),
        Money::FromAmount(104.00, Currency::Eur)
    };

    const auto returns = fin::estimation::LogReturns(closes);

    EXPECT_NEAR(returns[0], 0.019803, 5e-7);
    EXPECT_NEAR(returns[1], -0.009852, 5e-7);
    EXPECT_NEAR(returns[2], 0.029270, 5e-7);
    EXPECT_NEAR(returns[0] + returns[1] + returns[2], 0.039221, 5e-7);
    EXPECT_NEAR(HistoricalVolatility::From(returns, tradingDaysPerYear).Annualized(), 0.3240, 5e-5);
}

TEST_F(TestHistoricalVolatility, fewer_than_two_returns_stop_the_run)
{
    const std::vector<double> single{ 0.01 };

    EXPECT_DEATH(HistoricalVolatility::From(single, tradingDaysPerYear), "");
    EXPECT_DEATH(HistoricalVolatility::From({}, tradingDaysPerYear), "");
}

TEST_F(TestHistoricalVolatility, an_observation_frequency_that_is_not_positive_stops_the_run)
{
    const auto returns = Alternating(10, 0.01);

    EXPECT_DEATH(HistoricalVolatility::From(returns, 0.0), "");
    EXPECT_DEATH(HistoricalVolatility::From(returns, -252.0), "");
}
