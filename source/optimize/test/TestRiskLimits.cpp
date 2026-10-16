#include "source/optimize/RiskLimits.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::optimize::Binds;
    using fin::optimize::ExcessOver;
    using fin::optimize::IsFeasible;
    using fin::optimize::Penalty;
    using fin::optimize::RiskExcess;
    using fin::optimize::RiskLimits;
    using fin::sim::OutcomeDistribution;
    using fin::sim::PathOutcome;

    constexpr std::int32_t cycles = 4;

    RiskLimits Limits()
    {
        return { .maximumDrawdown = 0.30, .maximumAssignmentFrequency = 0.50, .penaltyWeight = 1.0 };
    }

    OutcomeDistribution Ran(std::int32_t assignments, double drawdown)
    {
        return OutcomeDistribution::Of({ { .terminalEquity = Money::FromAmount(120000.00, Currency::Eur), .growthRate = 0.07, .positions = cycles, .assignments = assignments, .maximumDrawdown = drawdown } });
    }
}

TEST(TestRiskLimits, limits_within_their_natural_bounds_are_well_formed)
{
    EXPECT_TRUE(IsWellFormed(Limits()));
    EXPECT_TRUE(IsWellFormed(RiskLimits{ .maximumDrawdown = 0.0, .maximumAssignmentFrequency = 1.0, .penaltyWeight = 0.0 }));
}

TEST(TestRiskLimits, limits_outside_their_natural_bounds_are_not_well_formed)
{
    EXPECT_FALSE(IsWellFormed(RiskLimits{ .maximumDrawdown = 1.5, .maximumAssignmentFrequency = 0.5, .penaltyWeight = 1.0 }));
    EXPECT_FALSE(IsWellFormed(RiskLimits{ .maximumDrawdown = -0.1, .maximumAssignmentFrequency = 0.5, .penaltyWeight = 1.0 }));
    EXPECT_FALSE(IsWellFormed(RiskLimits{ .maximumDrawdown = 0.3, .maximumAssignmentFrequency = 1.5, .penaltyWeight = 1.0 }));
    EXPECT_FALSE(IsWellFormed(RiskLimits{ .maximumDrawdown = 0.3, .maximumAssignmentFrequency = 0.5, .penaltyWeight = -1.0 }));
}

TEST(TestRiskLimits, an_outcome_that_stays_inside_both_limits_has_slack_and_is_feasible)
{
    const auto excess = ExcessOver(Limits(), Ran(1, 0.20));

    EXPECT_DOUBLE_EQ(excess.drawdown, -0.10);
    EXPECT_DOUBLE_EQ(excess.assignmentFrequency, -0.25);
    EXPECT_TRUE(IsFeasible(excess));
    EXPECT_FALSE(Binds(excess.drawdown));
    EXPECT_FALSE(Binds(excess.assignmentFrequency));
    EXPECT_DOUBLE_EQ(Penalty(Limits(), excess), 0.0);
}

TEST(TestRiskLimits, an_outcome_exactly_on_a_limit_binds_without_being_infeasible)
{
    const auto excess = ExcessOver(Limits(), Ran(2, 0.30));

    EXPECT_DOUBLE_EQ(excess.drawdown, 0.0);
    EXPECT_DOUBLE_EQ(excess.assignmentFrequency, 0.0);
    EXPECT_TRUE(IsFeasible(excess));
    EXPECT_TRUE(Binds(excess.drawdown));
    EXPECT_TRUE(Binds(excess.assignmentFrequency));
    EXPECT_DOUBLE_EQ(Penalty(Limits(), excess), 0.0);
}

TEST(TestRiskLimits, a_breach_of_either_limit_makes_an_outcome_infeasible)
{
    EXPECT_FALSE(IsFeasible(ExcessOver(Limits(), Ran(1, 0.45))));
    EXPECT_FALSE(IsFeasible(ExcessOver(Limits(), Ran(3, 0.20))));
    EXPECT_FALSE(IsFeasible(ExcessOver(Limits(), Ran(3, 0.45))));
}

TEST(TestRiskLimits, only_the_amount_by_which_a_limit_is_passed_is_charged_for)
{
    EXPECT_DOUBLE_EQ(Penalty(Limits(), ExcessOver(Limits(), Ran(1, 0.45))), 0.15);
    EXPECT_DOUBLE_EQ(Penalty(Limits(), ExcessOver(Limits(), Ran(3, 0.20))), 0.25);
    EXPECT_DOUBLE_EQ(Penalty(Limits(), ExcessOver(Limits(), Ran(3, 0.45))), 0.40);
}

TEST(TestRiskLimits, the_penalty_grows_with_the_breach_and_with_the_weight_it_carries)
{
    const auto shallow = ExcessOver(Limits(), Ran(1, 0.35));
    const auto deep = ExcessOver(Limits(), Ran(1, 0.55));

    EXPECT_LT(Penalty(Limits(), shallow), Penalty(Limits(), deep));
    EXPECT_DOUBLE_EQ(Penalty(RiskLimits{ .maximumDrawdown = 0.30, .maximumAssignmentFrequency = 0.50, .penaltyWeight = 4.0 }, deep), 4.0 * Penalty(Limits(), deep));
}

TEST(TestRiskLimits, a_weight_of_nothing_leaves_a_breach_uncharged)
{
    const RiskLimits ignored{ .maximumDrawdown = 0.30, .maximumAssignmentFrequency = 0.50, .penaltyWeight = 0.0 };

    EXPECT_FALSE(IsFeasible(ExcessOver(ignored, Ran(4, 0.90))));
    EXPECT_DOUBLE_EQ(Penalty(ignored, ExcessOver(ignored, Ran(4, 0.90))), 0.0);
}
