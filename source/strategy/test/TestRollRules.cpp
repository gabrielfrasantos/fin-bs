#include "source/strategy/RollRules.hpp"
#include <gtest/gtest.h>
#include <map>
#include <vector>

namespace
{
    using fin::core::Delta;
    using fin::strategy::CapturedBasisPoints;
    using fin::strategy::PositionObservation;
    using fin::strategy::RollReason;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;

    PositionObservation Observed(std::int32_t daysToExpiry, double valuePerShare, double delta)
    {
        return { .daysToExpiry = daysToExpiry, .premiumReceivedPerShare = 1.00, .valuePerShare = valuePerShare, .delta = Delta::FromValue(delta) };
    }

    RollRules Rolling(std::int32_t daysToExpiry, std::int32_t profitCaptureBasisPoints, double deltaLimit)
    {
        return RollRules(RollTriggers{ .daysToExpiry = daysToExpiry, .profitCaptureBasisPoints = profitCaptureBasisPoints, .deltaLimit = Delta::FromValue(deltaLimit) });
    }

    class TestRollRules
        : public ::testing::Test
    {
    protected:
        RollRules rules{ Rolling(7, 5000, 0.45) };
    };
}

TEST_F(TestRollRules, triggers_that_can_never_fire_or_always_fire_are_refused)
{
    EXPECT_DEATH(Rolling(-1, 5000, 0.45), "");
    EXPECT_DEATH(Rolling(7, 0, 0.45), "");
    EXPECT_DEATH(Rolling(7, 10001, 0.45), "");
    EXPECT_DEATH(Rolling(7, 5000, 0.0), "");
}

TEST_F(TestRollRules, a_position_with_time_left_profit_to_make_and_no_risk_is_left_alone)
{
    EXPECT_FALSE(rules.Triggered(Observed(30, 0.80, 0.20)).has_value());
}

TEST_F(TestRollRules, most_of_the_premium_earned_is_a_reason_to_roll)
{
    EXPECT_EQ(rules.Triggered(Observed(30, 0.40, 0.20)), RollReason::ProfitCaptured);
    EXPECT_EQ(rules.Triggered(Observed(30, 0.50, 0.20)), RollReason::ProfitCaptured);
    EXPECT_FALSE(rules.Triggered(Observed(30, 0.51, 0.20)).has_value());
}

TEST_F(TestRollRules, running_out_of_time_is_a_reason_to_roll)
{
    EXPECT_EQ(rules.Triggered(Observed(7, 0.80, 0.20)), RollReason::NearExpiry);
    EXPECT_EQ(rules.Triggered(Observed(0, 0.80, 0.20)), RollReason::NearExpiry);
    EXPECT_FALSE(rules.Triggered(Observed(8, 0.80, 0.20)).has_value());
}

TEST_F(TestRollRules, a_strike_the_price_has_come_to_meet_is_a_reason_to_roll)
{
    EXPECT_EQ(rules.Triggered(Observed(30, 0.80, 0.45)), RollReason::DeltaBreached);
    EXPECT_EQ(rules.Triggered(Observed(30, 0.80, -0.60)), RollReason::DeltaBreached);
    EXPECT_FALSE(rules.Triggered(Observed(30, 0.80, 0.44)).has_value());
}

TEST_F(TestRollRules, risk_is_reported_ahead_of_profit_and_profit_ahead_of_time)
{
    EXPECT_EQ(rules.Triggered(Observed(3, 0.10, 0.60)), RollReason::DeltaBreached);
    EXPECT_EQ(rules.Triggered(Observed(3, 0.10, 0.20)), RollReason::ProfitCaptured);
}

TEST_F(TestRollRules, each_trigger_fires_once_along_a_scripted_path)
{
    const std::vector<PositionObservation> path{
        Observed(45, 0.95, 0.15),
        Observed(38, 0.45, 0.20),
        Observed(30, 0.90, 0.55),
        Observed(20, 0.80, 0.30),
        Observed(7, 0.70, 0.30)
    };

    std::map<RollReason, int> fired;

    for (const auto& observation : path)
        if (const auto reason = rules.Triggered(observation))
            ++fired[*reason];

    EXPECT_EQ(fired[RollReason::ProfitCaptured], 1);
    EXPECT_EQ(fired[RollReason::DeltaBreached], 1);
    EXPECT_EQ(fired[RollReason::NearExpiry], 1);
}

TEST_F(TestRollRules, a_replacement_may_not_carry_more_risk_than_the_cap_allows)
{
    EXPECT_TRUE(rules.Allows(Delta::FromValue(0.30)));
    EXPECT_TRUE(rules.Allows(Delta::FromValue(0.45)));
    EXPECT_TRUE(rules.Allows(Delta::FromValue(-0.45)));
    EXPECT_FALSE(rules.Allows(Delta::FromValue(0.46)));
    EXPECT_FALSE(rules.Allows(Delta::FromValue(-0.90)));
}

TEST_F(TestRollRules, what_has_been_captured_is_the_premium_that_is_no_longer_owed)
{
    EXPECT_EQ(CapturedBasisPoints(Observed(30, 0.00, 0.20)), 10000);
    EXPECT_EQ(CapturedBasisPoints(Observed(30, 0.25, 0.20)), 7500);
    EXPECT_EQ(CapturedBasisPoints(Observed(30, 1.00, 0.20)), 0);
    EXPECT_EQ(CapturedBasisPoints(Observed(30, 3.00, 0.20)), -10000);
}

TEST_F(TestRollRules, a_position_that_was_never_paid_for_cannot_be_judged_on_profit)
{
    EXPECT_DEATH(rules.Triggered({ .daysToExpiry = 30, .premiumReceivedPerShare = 0.0, .valuePerShare = 0.10, .delta = Delta::FromValue(0.20) }), "");
    EXPECT_DEATH(rules.Triggered({ .daysToExpiry = -1, .premiumReceivedPerShare = 1.0, .valuePerShare = 0.10, .delta = Delta::FromValue(0.20) }), "");
    EXPECT_DEATH(rules.Triggered({ .daysToExpiry = 30, .premiumReceivedPerShare = 1.0, .valuePerShare = -0.10, .delta = Delta::FromValue(0.20) }), "");
}
