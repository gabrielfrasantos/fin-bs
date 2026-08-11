#include "source/core/Volatility.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    using fin::core::Volatility;

    class TestVolatility
        : public ::testing::Test
    {
    protected:
        static constexpr double epsilon = 1e-12;
    };
}

TEST_F(TestVolatility, exposes_the_variance_of_the_annualised_deviation)
{
    EXPECT_NEAR(Volatility::FromAnnualized(0.2).Variance(), 0.04, epsilon);
    EXPECT_NEAR(Volatility::Zero().Variance(), 0.0, epsilon);
}

TEST_F(TestVolatility, round_trips_between_deviation_and_variance)
{
    for (auto annualized : { 0.0, 0.05, 0.2, 0.85 })
        EXPECT_NEAR(Volatility::FromVariance(Volatility::FromAnnualized(annualized).Variance()).Annualized(), annualized, 1e-15);
}

TEST_F(TestVolatility, annualises_a_period_deviation_by_the_square_root_of_the_period_count)
{
    EXPECT_NEAR(Volatility::FromPeriodDeviation(0.0125, 252.0).Annualized(), 0.0125 * std::sqrt(252.0), epsilon);
    EXPECT_NEAR(Volatility::FromPeriodDeviation(0.0125, 252.0).Annualized(), 0.198431, 1e-6);
}

TEST_F(TestVolatility, scales_to_a_horizon_by_the_square_root_of_time)
{
    const auto volatility = Volatility::FromPeriodDeviation(0.0125, 252.0);

    EXPECT_NEAR(volatility.OverHorizon(30.0 / 365.0), 0.056888, 1e-6);
    EXPECT_NEAR(volatility.OverHorizon(1.0), volatility.Annualized(), epsilon);
    EXPECT_NEAR(volatility.OverHorizon(0.0), 0.0, epsilon);
}

TEST_F(TestVolatility, variance_is_additive_over_consecutive_horizons)
{
    const auto volatility = Volatility::FromAnnualized(0.2);
    const auto first = volatility.OverHorizon(0.3);
    const auto second = volatility.OverHorizon(0.7);
    const auto whole = volatility.OverHorizon(1.0);

    EXPECT_NEAR(first * first + second * second, whole * whole, 1e-15);
}

TEST_F(TestVolatility, orders_by_the_underlying_deviation)
{
    EXPECT_TRUE(Volatility::FromAnnualized(0.1) < Volatility::FromAnnualized(0.2));
    EXPECT_TRUE(Volatility::FromAnnualized(0.2) == Volatility::FromAnnualized(0.2));
}

TEST_F(TestVolatility, is_usable_in_a_constant_expression)
{
    static_assert(Volatility::FromAnnualized(0.2).Annualized() == 0.2);
    static_assert(Volatility::Zero().Variance() == 0.0);
}

TEST_F(TestVolatility, rejects_negative_inputs)
{
    EXPECT_DEATH(static_cast<void>(Volatility::FromAnnualized(-0.01)), "");
    EXPECT_DEATH(static_cast<void>(Volatility::FromVariance(-0.01)), "");
    EXPECT_DEATH(static_cast<void>(Volatility::FromPeriodDeviation(-0.01, 252.0)), "");
    EXPECT_DEATH(static_cast<void>(Volatility::FromPeriodDeviation(0.01, 0.0)), "");
    EXPECT_DEATH(static_cast<void>(Volatility::FromAnnualized(0.2).OverHorizon(-1.0)), "");
}
