#include "source/estimation/DriftEstimator.hpp"
#include "source/estimation/HistoricalVolatility.hpp"
#include "source/estimation/MarketParameters.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "source/estimation/TotalLeastSquaresFit.hpp"
#include "source/estimation/test_doubles/GbmSeries.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::estimation::DriftEstimator;
    using fin::estimation::HistoricalVolatility;
    using fin::estimation::MarketParameters;
    using fin::estimation::OrdinaryLeastSquaresFit;
    using fin::estimation::PriceSeries;
    using fin::estimation::TotalLeastSquaresFit;
    using fin::estimation::test_doubles::GbmSeries;

    class TestMarketParameters
        : public ::testing::Test
    {
    protected:
        OrdinaryLeastSquaresFit ordinary;
        TotalLeastSquaresFit orthogonal;

        PriceSeries Series(double drift, double volatility, uint64_t seed, std::size_t observations) const
        {
            GbmSeries generator(GbmSeries::Parameters{
                                    .start = Date::FromYearMonthDay(1998, 1, 5),
                                    .initialPrice = Money::FromAmount(100.0, Currency::Eur),
                                    .drift = Rate::FromContinuous(drift),
                                    .volatility = Volatility::FromAnnualized(volatility) },
                seed);

            return generator.Generate(observations);
        }

        const Rate noDividend{ Rate::Zero() };
    };
}

TEST_F(TestMarketParameters, the_volatility_is_the_historical_estimate_of_the_series)
{
    const auto series = Series(0.06, 0.24, 1234u, 3000);
    const auto parameters = MarketParameters::From(series, noDividend, ordinary);

    ASSERT_TRUE(parameters.has_value());
    EXPECT_EQ(parameters->Volatility(), HistoricalVolatility::From(series.LogReturns(), series.PeriodsPerYear()));
}

TEST_F(TestMarketParameters, the_drift_is_the_fitted_log_drift_corrected_for_the_variance_of_the_path)
{
    const auto series = Series(0.06, 0.24, 1234u, 3000);
    const auto parameters = MarketParameters::From(series, noDividend, ordinary);
    const auto logDrift = DriftEstimator(ordinary).LogDrift(series);

    ASSERT_TRUE(parameters.has_value());
    ASSERT_TRUE(logDrift.has_value());
    EXPECT_DOUBLE_EQ(parameters->Drift().Continuous(), logDrift->Continuous() + 0.5 * parameters->Volatility().Variance());
}

TEST_F(TestMarketParameters, the_supplied_line_fit_is_the_one_that_is_used)
{
    const auto series = Series(0.06, 0.24, 1234u, 3000);
    const auto parameters = MarketParameters::From(series, noDividend, orthogonal);
    const auto logDrift = DriftEstimator(orthogonal).LogDrift(series);

    ASSERT_TRUE(parameters.has_value());
    ASSERT_TRUE(logDrift.has_value());
    EXPECT_DOUBLE_EQ(parameters->Drift().Continuous(), logDrift->Continuous() + 0.5 * parameters->Volatility().Variance());
}

TEST_F(TestMarketParameters, the_dividend_yield_is_carried_through_unchanged)
{
    const auto series = Series(0.06, 0.24, 1234u, 3000);
    const auto yield = Rate::FromContinuous(0.031);
    const auto parameters = MarketParameters::From(series, yield, ordinary);

    ASSERT_TRUE(parameters.has_value());
    EXPECT_EQ(parameters->DividendYield(), yield);
}

TEST_F(TestMarketParameters, the_parameters_of_a_synthetic_series_are_the_ones_it_was_generated_with)
{
    const auto parameters = MarketParameters::From(Series(0.09, 0.06, 55555u, 8000), noDividend, ordinary);

    ASSERT_TRUE(parameters.has_value());
    EXPECT_NEAR(parameters->Drift().Continuous(), 0.09, 0.03);
    EXPECT_NEAR(parameters->Volatility().Annualized(), 0.06, 0.05 * 0.06);
}

TEST_F(TestMarketParameters, a_negative_dividend_yield_stops_the_run)
{
    const auto series = Series(0.06, 0.24, 1234u, 500);

    EXPECT_DEATH(MarketParameters::From(series, Rate::FromContinuous(-0.01), ordinary), "");
}
