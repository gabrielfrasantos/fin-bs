#include "source/core/Rate.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    using fin::core::Rate;

    class TestRate
        : public ::testing::Test
    {
    protected:
        static constexpr double epsilon = 1e-12;
    };
}

TEST_F(TestRate, converts_an_effective_annual_rate_to_the_continuous_convention)
{
    EXPECT_NEAR(Rate::FromEffectiveAnnual(0.04).Continuous(), 0.039220713153281, 1e-12);
    EXPECT_NEAR(Rate::FromEffectiveAnnual(0.20).Continuous(), 0.182321556793955, 1e-12);
    EXPECT_NEAR(Rate::FromEffectiveAnnual(0.0).Continuous(), 0.0, epsilon);
}

TEST_F(TestRate, round_trips_between_the_two_conventions)
{
    for (auto effectiveAnnual : { -0.005, 0.0, 0.02, 0.04, 0.20, 1.5 })
    {
        const auto rate = Rate::FromEffectiveAnnual(effectiveAnnual);

        EXPECT_NEAR(rate.EffectiveAnnual(), effectiveAnnual, 1e-14);
        EXPECT_NEAR(Rate::FromContinuous(rate.Continuous()).EffectiveAnnual(), effectiveAnnual, 1e-14);
    }
}

TEST_F(TestRate, the_continuous_rate_is_below_the_effective_annual_rate_when_positive)
{
    const auto rate = Rate::FromEffectiveAnnual(0.20);

    EXPECT_LT(rate.Continuous(), rate.EffectiveAnnual());
}

TEST_F(TestRate, a_zero_rate_neither_grows_nor_discounts)
{
    EXPECT_DOUBLE_EQ(Rate::Zero().GrowthFactor(5.0), 1.0);
    EXPECT_DOUBLE_EQ(Rate::Zero().DiscountFactor(5.0), 1.0);
}

TEST_F(TestRate, growth_and_discount_factors_are_inverses)
{
    const auto rate = Rate::FromEffectiveAnnual(0.04);

    EXPECT_NEAR(rate.GrowthFactor(0.75) * rate.DiscountFactor(0.75), 1.0, 1e-14);
}

TEST_F(TestRate, growth_factors_compose_over_consecutive_horizons)
{
    const auto rate = Rate::FromEffectiveAnnual(0.04);

    EXPECT_NEAR(rate.GrowthFactor(0.3) * rate.GrowthFactor(0.7), rate.GrowthFactor(1.0), 1e-14);
}

TEST_F(TestRate, one_year_growth_matches_the_effective_annual_definition)
{
    const auto rate = Rate::FromEffectiveAnnual(0.04);

    EXPECT_NEAR(rate.GrowthFactor(1.0), 1.04, 1e-14);
    EXPECT_NEAR(rate.GrowthFactor(30.0), std::pow(1.04, 30.0), 1e-12);
}

TEST_F(TestRate, discounts_a_strike_due_in_forty_five_days)
{
    const auto rate = Rate::FromEffectiveAnnual(0.04);

    EXPECT_NEAR(100.0 * rate.DiscountFactor(45.0 / 365.0), 99.51763, 1e-5);
}

TEST_F(TestRate, supports_negative_rates)
{
    const auto rate = Rate::FromEffectiveAnnual(-0.005);

    EXPECT_LT(rate.Continuous(), 0.0);
    EXPECT_LT(rate.GrowthFactor(1.0), 1.0);
    EXPECT_GT(rate.DiscountFactor(1.0), 1.0);
}

TEST_F(TestRate, orders_by_the_underlying_rate)
{
    EXPECT_TRUE(Rate::FromContinuous(0.01) < Rate::FromContinuous(0.02));
    EXPECT_TRUE(Rate::FromContinuous(0.02) == Rate::FromContinuous(0.02));
}

TEST_F(TestRate, rejects_an_effective_annual_rate_of_minus_one_hundred_percent_or_below)
{
    EXPECT_DEATH(static_cast<void>(Rate::FromEffectiveAnnual(-1.0)), "");
    EXPECT_DEATH(static_cast<void>(Rate::FromEffectiveAnnual(-1.5)), "");
}
