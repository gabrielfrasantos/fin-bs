#include "source/estimation/DriftEstimator.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "source/estimation/TotalLeastSquaresFit.hpp"
#include "source/estimation/test_doubles/GbmSeries.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::estimation::DriftEstimator;
    using fin::estimation::LineFit;
    using fin::estimation::OrdinaryLeastSquaresFit;
    using fin::estimation::PriceSeries;
    using fin::estimation::TotalLeastSquaresFit;
    using fin::estimation::test_doubles::GbmSeries;

    class TestDriftEstimator
        : public ::testing::Test
    {
    protected:
        OrdinaryLeastSquaresFit ordinary;
        TotalLeastSquaresFit orthogonal;

        std::vector<const LineFit*> Both() const
        {
            return { &ordinary, &orthogonal };
        }

        PriceSeries Series(double drift, double volatility, uint64_t seed, std::size_t observations, double initialPrice = 100.0) const
        {
            GbmSeries generator(GbmSeries::Parameters{
                                    .start = Date::FromYearMonthDay(1995, 1, 2),
                                    .initialPrice = Money::FromAmount(initialPrice, Currency::Eur),
                                    .drift = Rate::FromContinuous(drift),
                                    .volatility = Volatility::FromAnnualized(volatility) },
                seed);

            return generator.Generate(observations);
        }

        PriceSeries Scaled(const PriceSeries& series, int64_t factor) const
        {
            std::vector<Date> dates(series.Dates().begin(), series.Dates().end());
            std::vector<Money> closes;

            for (const auto& close : series.Closes())
                closes.push_back(close * factor);

            return PriceSeries::From(std::move(dates), std::move(closes), series.Convention());
        }

        PriceSeries Flat(std::size_t observations) const
        {
            return Series(0.0, 0.0, 1u, observations);
        }

        double LogDriftOf(const PriceSeries& series, const LineFit& fit) const
        {
            const auto estimated = DriftEstimator(fit).LogDrift(series);

            EXPECT_TRUE(estimated.has_value());

            return estimated->Continuous();
        }
    };
}

TEST_F(TestDriftEstimator, a_path_without_volatility_is_a_straight_line_whose_slope_is_the_drift)
{
    const auto series = Series(0.10, 0.0, 3u, 2000);

    for (const auto* fit : Both())
        EXPECT_NEAR(LogDriftOf(series, *fit), 0.10, 1e-4);
}

TEST_F(TestDriftEstimator, a_falling_series_has_a_negative_drift)
{
    const auto series = Series(-0.06, 0.0, 5u, 1000);

    EXPECT_NEAR(LogDriftOf(series, ordinary), -0.06, 1e-4);
}

TEST_F(TestDriftEstimator, a_price_that_never_moves_has_no_drift)
{
    const auto series = Flat(500);

    for (const auto* fit : Both())
        EXPECT_EQ(LogDriftOf(series, *fit), 0.0);
}

TEST_F(TestDriftEstimator, a_known_drift_is_recovered_from_a_synthetic_price_series)
{
    const auto series = Series(0.08, 0.05, 987654u, 8000);
    const auto expected = 0.08 - 0.5 * 0.05 * 0.05;

    for (const auto* fit : Both())
        EXPECT_NEAR(LogDriftOf(series, *fit), expected, 0.03);
}

TEST_F(TestDriftEstimator, the_two_methods_agree_when_the_path_is_almost_deterministic)
{
    const auto series = Series(0.09, 0.005, 246810u, 4000);
    const auto ordinarySlope = LogDriftOf(series, ordinary);

    EXPECT_NEAR(LogDriftOf(series, orthogonal), ordinarySlope, 1e-3 * std::abs(ordinarySlope));
}

TEST_F(TestDriftEstimator, orthogonal_regression_fits_a_steeper_line_when_the_path_is_noisy)
{
    const auto series = Series(0.09, 0.18, 246810u, 4000);

    EXPECT_GT(LogDriftOf(series, orthogonal), LogDriftOf(series, ordinary));
}

TEST_F(TestDriftEstimator, the_drift_does_not_change_when_the_price_is_quoted_in_a_different_size)
{
    const auto series = Series(0.07, 0.15, 13579u, 3000, 1000.0);
    const auto scaled = Scaled(series, 7);

    for (const auto* fit : Both())
        EXPECT_NEAR(LogDriftOf(scaled, *fit), LogDriftOf(series, *fit), 1e-5);
}

TEST_F(TestDriftEstimator, a_longer_observation_window_narrows_the_error_of_the_estimate)
{
    const auto expected = 0.08 - 0.5 * 0.2 * 0.2;

    double previous = 1.0;
    for (auto observations : { std::size_t(500), std::size_t(2000), std::size_t(8000) })
    {
        const auto error = std::abs(LogDriftOf(Series(0.08, 0.2, 111u, observations), ordinary) - expected);

        EXPECT_LT(error, previous);
        previous = error;
    }
}

TEST_F(TestDriftEstimator, the_documented_worked_example_is_reproduced)
{
    const auto start = Date::FromYearMonthDay(2024, 1, 1);
    std::vector<Date> dates;
    std::vector<Money> closes;

    for (auto amount : { 100.00, 100.85, 101.71, 102.58, 103.46 })
    {
        dates.push_back(start.AddDays(31 * static_cast<int32_t>(dates.size())));
        closes.push_back(Money::FromAmount(amount, Currency::Eur));
    }

    const auto series = PriceSeries::From(std::move(dates), std::move(closes), DayCountConvention::Act365Fixed);

    EXPECT_NEAR(LogDriftOf(series, ordinary), 0.100126, 5e-7);
}

TEST_F(TestDriftEstimator, a_series_longer_than_the_fit_admits_stops_the_run)
{
    const auto series = Series(0.05, 0.1, 2u, ordinary.MaximumObservations() + 1);

    EXPECT_DEATH(DriftEstimator(ordinary).LogDrift(series), "");
}
