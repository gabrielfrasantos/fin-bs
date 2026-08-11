#include "source/estimation/EmaVolatilityEstimator.hpp"
#include "source/estimation/HistoricalVolatility.hpp"
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
    using fin::estimation::EmaVolatilityEstimator;
    using fin::estimation::HistoricalVolatility;
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

    class TestEmaVolatilityEstimator
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

TEST_F(TestEmaVolatilityEstimator, a_smoothing_factor_of_one_forgets_everything_but_the_latest_return)
{
    const auto estimator = EmaVolatilityEstimator::WithSmoothingFactor(1.0, tradingDaysPerYear);
    const std::vector<double> returns{ 0.05, -0.02, 0.011 };

    EXPECT_NEAR(estimator.From(returns).Annualized(), AnnualizedDeviationOf(0.011 * 0.011), 1e-15);
}

TEST_F(TestEmaVolatilityEstimator, a_series_of_constant_magnitude_is_reported_as_that_magnitude)
{
    const auto estimator = EmaVolatilityEstimator::WithSmoothingFactor(0.05, tradingDaysPerYear);

    EXPECT_NEAR(estimator.From(Repeated(500, 0.01)).Annualized(), AnnualizedDeviationOf(0.0001), 1e-15);
}

TEST_F(TestEmaVolatilityEstimator, a_step_in_magnitude_decays_geometrically_towards_the_new_level)
{
    constexpr double smoothingFactor = 0.04;
    const auto estimator = EmaVolatilityEstimator::WithSmoothingFactor(smoothingFactor, tradingDaysPerYear);

    for (auto elapsed : { std::size_t(1), std::size_t(10), std::size_t(50), std::size_t(200) })
    {
        const auto returns = Stepped(100, 0.01, elapsed, 0.02);
        const auto remaining = std::pow(1.0 - smoothingFactor, static_cast<double>(elapsed));
        const auto expected = 0.0004 + (0.0001 - 0.0004) * remaining;

        EXPECT_NEAR(estimator.From(returns).Annualized(), AnnualizedDeviationOf(expected), 1e-12);
    }
}

TEST_F(TestEmaVolatilityEstimator, the_half_life_is_the_number_of_periods_that_halves_the_remaining_gap)
{
    constexpr double halfLife = 20.0;
    const auto estimator = EmaVolatilityEstimator::WithHalfLife(halfLife, tradingDaysPerYear);

    const auto afterOneHalfLife = estimator.From(Stepped(100, 0.01, 20, 0.02));
    const auto afterTwoHalfLives = estimator.From(Stepped(100, 0.01, 40, 0.02));

    EXPECT_NEAR(afterOneHalfLife.Variance() / tradingDaysPerYear, 0.0004 - 0.5 * 0.0003, 1e-15);
    EXPECT_NEAR(afterTwoHalfLives.Variance() / tradingDaysPerYear, 0.0004 - 0.25 * 0.0003, 1e-15);
}

TEST_F(TestEmaVolatilityEstimator, a_shorter_half_life_reaches_the_new_level_sooner)
{
    const auto reactive = EmaVolatilityEstimator::WithHalfLife(5.0, tradingDaysPerYear);
    const auto sluggish = EmaVolatilityEstimator::WithHalfLife(80.0, tradingDaysPerYear);

    EXPECT_GT(reactive.SmoothingFactor(), sluggish.SmoothingFactor());
    EXPECT_GT(reactive.From(Stepped(100, 0.01, 20, 0.02)).Annualized(), sluggish.From(Stepped(100, 0.01, 20, 0.02)).Annualized());
}

TEST_F(TestEmaVolatilityEstimator, the_track_reports_one_estimate_per_return_and_ends_where_the_single_estimate_does)
{
    const auto estimator = EmaVolatilityEstimator::WithSmoothingFactor(0.1, tradingDaysPerYear);
    const auto returns = Stepped(30, 0.005, 30, 0.02);
    const auto track = estimator.Track(returns);

    EXPECT_EQ(track.size(), returns.size());
    EXPECT_EQ(track.front(), Volatility::FromPeriodDeviation(0.005, tradingDaysPerYear));
    EXPECT_EQ(track.back(), estimator.From(returns));
}

TEST_F(TestEmaVolatilityEstimator, the_track_rises_monotonically_after_a_step_up)
{
    const auto estimator = EmaVolatilityEstimator::WithSmoothingFactor(0.1, tradingDaysPerYear);
    const auto track = estimator.Track(Stepped(30, 0.005, 30, 0.02));

    for (std::size_t i = 31; i != track.size(); ++i)
        EXPECT_GT(track[i], track[i - 1]);
}

TEST_F(TestEmaVolatilityEstimator, on_a_stationary_series_it_settles_near_the_historical_estimate)
{
    GbmSeries generator(GbmSeries::Parameters{
                            .start = Date::FromYearMonthDay(2004, 1, 5),
                            .initialPrice = Money::FromAmount(100.0, Currency::Eur),
                            .drift = Rate::FromContinuous(0.05),
                            .volatility = Volatility::FromAnnualized(0.22) },
        7654321u);

    const auto series = generator.Generate(5000);
    const auto returns = series.LogReturns();
    const auto estimator = EmaVolatilityEstimator::WithHalfLife(250.0, series.PeriodsPerYear());
    const auto historical = HistoricalVolatility::From(returns, series.PeriodsPerYear());

    EXPECT_NEAR(estimator.From(returns).Annualized(), historical.Annualized(), 0.15 * historical.Annualized());
}

TEST_F(TestEmaVolatilityEstimator, the_documented_worked_example_is_reproduced)
{
    const auto estimator = EmaVolatilityEstimator::WithHalfLife(20.0, tradingDaysPerYear);

    EXPECT_NEAR(estimator.SmoothingFactor(), 0.034064, 5e-7);
    EXPECT_NEAR(estimator.From(Stepped(400, 0.01, 1, 0.01)).Annualized(), 0.1587, 5e-5);
    EXPECT_NEAR(estimator.From(Stepped(400, 0.01, 10, 0.02)).Annualized(), 0.2176, 5e-5);
    EXPECT_NEAR(estimator.From(Stepped(400, 0.01, 20, 0.02)).Annualized(), 0.2510, 5e-5);
    EXPECT_NEAR(estimator.From(Stepped(400, 0.01, 40, 0.02)).Annualized(), 0.2862, 5e-5);
    EXPECT_NEAR(estimator.From(Stepped(400, 0.01, 100, 0.02)).Annualized(), 0.3137, 5e-5);
}

TEST_F(TestEmaVolatilityEstimator, a_smoothing_factor_outside_its_admissible_range_stops_the_run)
{
    EXPECT_DEATH(EmaVolatilityEstimator::WithSmoothingFactor(0.0, tradingDaysPerYear), "");
    EXPECT_DEATH(EmaVolatilityEstimator::WithSmoothingFactor(-0.1, tradingDaysPerYear), "");
    EXPECT_DEATH(EmaVolatilityEstimator::WithSmoothingFactor(1.5, tradingDaysPerYear), "");
}

TEST_F(TestEmaVolatilityEstimator, a_half_life_that_is_not_positive_stops_the_run)
{
    EXPECT_DEATH(EmaVolatilityEstimator::WithHalfLife(0.0, tradingDaysPerYear), "");
    EXPECT_DEATH(EmaVolatilityEstimator::WithHalfLife(-10.0, tradingDaysPerYear), "");
}

TEST_F(TestEmaVolatilityEstimator, an_observation_frequency_that_is_not_positive_stops_the_run)
{
    EXPECT_DEATH(EmaVolatilityEstimator::WithSmoothingFactor(0.1, 0.0), "");
}

TEST_F(TestEmaVolatilityEstimator, an_empty_series_stops_the_run)
{
    const auto estimator = EmaVolatilityEstimator::WithSmoothingFactor(0.1, tradingDaysPerYear);

    EXPECT_DEATH(estimator.From({}), "");
    EXPECT_DEATH(estimator.Track({}), "");
}
