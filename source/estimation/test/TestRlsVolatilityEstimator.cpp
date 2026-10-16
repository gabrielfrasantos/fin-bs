#include "source/estimation/HistoricalVolatility.hpp"
#include "source/estimation/RlsVolatilityEstimator.hpp"
#include "source/estimation/test_doubles/GbmSeries.hpp"
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
    using fin::estimation::RlsVolatilityEstimator;
    using fin::estimation::test_doubles::GbmSeries;

    std::vector<double> Repeated(std::size_t count, double magnitude)
    {
        std::vector<double> result;

        for (std::size_t i = 0; i != count; ++i)
            result.push_back(i % 2 == 0 ? magnitude : -magnitude);

        return result;
    }

    std::vector<double> Stepped(std::size_t before, double first, std::size_t after, double second)
    {
        auto result = Repeated(before, first);
        const auto tail = Repeated(after, second);

        result.insert(result.end(), tail.begin(), tail.end());

        return result;
    }

    class TestRlsVolatilityEstimator
        : public ::testing::Test
    {
    protected:
        static constexpr double tradingDaysPerYear = 252.0;

        double AnnualizedDeviationOf(double periodVariance) const
        {
            return std::sqrt(periodVariance * tradingDaysPerYear);
        }
    };
}

TEST_F(TestRlsVolatilityEstimator, without_forgetting_the_estimate_is_the_mean_squared_return)
{
    const auto estimator = RlsVolatilityEstimator::WithForgettingFactor(1.0, tradingDaysPerYear);
    const auto returns = Stepped(600, 0.01, 400, 0.03);

    double sumOfSquares = 0.0;
    for (auto logReturn : returns)
        sumOfSquares += logReturn * logReturn;

    const auto expected = AnnualizedDeviationOf(sumOfSquares / static_cast<double>(returns.size()));

    EXPECT_NEAR(estimator.From(returns).Annualized(), expected, 1e-8 * expected);
}

TEST_F(TestRlsVolatilityEstimator, without_forgetting_it_converges_to_the_historical_estimate)
{
    GbmSeries generator(GbmSeries::Parameters{
                            .start = Date::FromYearMonthDay(2001, 1, 2),
                            .initialPrice = Money::FromAmount(100.0, Currency::Eur),
                            .drift = Rate::FromContinuous(0.06),
                            .volatility = Volatility::FromAnnualized(0.28) },
        424242u);

    const auto series = generator.Generate(5000);
    const auto returns = series.LogReturns();
    const auto estimator = RlsVolatilityEstimator::WithForgettingFactor(1.0, series.PeriodsPerYear());
    const auto historical = HistoricalVolatility::From(returns, series.PeriodsPerYear());

    EXPECT_NEAR(estimator.From(returns).Annualized(), historical.Annualized(), 0.001 * historical.Annualized());
}

TEST_F(TestRlsVolatilityEstimator, a_series_of_constant_magnitude_is_reported_as_that_magnitude)
{
    for (auto forgettingFactor : { 0.9, 0.98, 1.0 })
    {
        const auto estimator = RlsVolatilityEstimator::WithForgettingFactor(forgettingFactor, tradingDaysPerYear);

        EXPECT_NEAR(estimator.From(Repeated(2000, 0.012)).Annualized(), AnnualizedDeviationOf(0.012 * 0.012), 1e-9);
    }
}

TEST_F(TestRlsVolatilityEstimator, forgetting_lets_the_estimate_follow_a_regime_switch)
{
    const auto returns = Stepped(1000, 0.01, 200, 0.02);
    const auto forgetful = RlsVolatilityEstimator::WithForgettingFactor(0.98, tradingDaysPerYear);
    const auto retentive = RlsVolatilityEstimator::WithForgettingFactor(1.0, tradingDaysPerYear);

    EXPECT_NEAR(forgetful.From(returns).Annualized(), AnnualizedDeviationOf(0.0004), 0.05 * AnnualizedDeviationOf(0.0004));
    EXPECT_LT(retentive.From(returns).Annualized(), 0.7 * AnnualizedDeviationOf(0.0004));
}

TEST_F(TestRlsVolatilityEstimator, forgetting_faster_follows_the_regime_switch_sooner)
{
    const auto returns = Stepped(1000, 0.01, 60, 0.02);
    const auto faster = RlsVolatilityEstimator::WithForgettingFactor(0.95, tradingDaysPerYear);
    const auto slower = RlsVolatilityEstimator::WithForgettingFactor(0.995, tradingDaysPerYear);

    EXPECT_GT(faster.From(returns).Annualized(), slower.From(returns).Annualized());
    EXPECT_GT(faster.ForgettingFactor(), 0.0);
}

TEST_F(TestRlsVolatilityEstimator, the_track_reports_one_estimate_per_return_and_ends_where_the_single_estimate_does)
{
    const auto estimator = RlsVolatilityEstimator::WithForgettingFactor(0.97, tradingDaysPerYear);
    const auto returns = Stepped(50, 0.004, 50, 0.02);
    const auto track = estimator.Track(returns);

    EXPECT_EQ(track.size(), returns.size());
    EXPECT_EQ(track.back(), estimator.From(returns));
}

TEST_F(TestRlsVolatilityEstimator, the_first_estimate_of_the_track_is_the_first_return_itself)
{
    const auto estimator = RlsVolatilityEstimator::WithForgettingFactor(0.97, tradingDaysPerYear);
    const auto track = estimator.Track(Repeated(10, 0.004));

    EXPECT_NEAR(track.front().Annualized(), AnnualizedDeviationOf(0.004 * 0.004), 1e-6);
}

TEST_F(TestRlsVolatilityEstimator, a_forgetting_factor_outside_its_admissible_range_stops_the_run)
{
    EXPECT_DEATH(RlsVolatilityEstimator::WithForgettingFactor(0.0, tradingDaysPerYear), "");
    EXPECT_DEATH(RlsVolatilityEstimator::WithForgettingFactor(-0.5, tradingDaysPerYear), "");
    EXPECT_DEATH(RlsVolatilityEstimator::WithForgettingFactor(1.1, tradingDaysPerYear), "");
}

TEST_F(TestRlsVolatilityEstimator, an_observation_frequency_that_is_not_positive_stops_the_run)
{
    EXPECT_DEATH(RlsVolatilityEstimator::WithForgettingFactor(0.97, 0.0), "");
}

TEST_F(TestRlsVolatilityEstimator, an_empty_series_stops_the_run)
{
    const auto estimator = RlsVolatilityEstimator::WithForgettingFactor(0.97, tradingDaysPerYear);

    EXPECT_DEATH(estimator.From({}), "");
    EXPECT_DEATH(estimator.Track({}), "");
}
