#include "source/optimize/Recommendation.hpp"
#include "source/optimize/test/StrategyScenario.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::optimize::DescentTerms;
    using fin::optimize::IsFeasible;
    using fin::optimize::Recommend;
    using fin::optimize::Recommendation;
    using fin::optimize::RiskLimits;
    using fin::optimize::StrategyObjective;
    using fin::optimize::StrategySetting;
    using fin::optimize::StrikeSizingOptimizer;
    using fin::optimize::test::Experiment;
    using fin::optimize::test::SearchedRange;
    using fin::optimize::test::Setting;
    using fin::optimize::test::UnreachableLimits;

    constexpr std::int32_t paths = 64;
    constexpr auto drawdownAllowed = 0.40;
    constexpr auto assignmentAllowed = 0.10;

    StrategySetting Start()
    {
        return Setting(0.30, 2200, 0.60);
    }

    StrikeSizingOptimizer Optimizer()
    {
        return StrikeSizingOptimizer(DescentTerms{ .learningRate = 4.0, .iterations = 24, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 });
    }

    Recommendation Under(const RiskLimits& limits)
    {
        return Recommend(Start(), StrategyObjective(Experiment(paths), SearchedRange(), limits), Optimizer());
    }

    Recommendation Unconstrained()
    {
        return Under(UnreachableLimits());
    }

    RiskLimits ShallowDrawdown()
    {
        return { .maximumDrawdown = drawdownAllowed, .maximumAssignmentFrequency = 1.0, .penaltyWeight = 1.0 };
    }

    RiskLimits FewAssignments()
    {
        return { .maximumDrawdown = 1.0, .maximumAssignmentFrequency = assignmentAllowed, .penaltyWeight = 1.0 };
    }
}

TEST(TestRecommendation, the_same_experiment_is_answered_the_same_way_every_time_it_is_run)
{
    const auto first = Unconstrained();
    const auto second = Unconstrained();

    EXPECT_EQ(first.setting.targetDelta, second.setting.targetDelta);
    EXPECT_EQ(first.setting.cashBufferBasisPoints, second.setting.cashBufferBasisPoints);
    EXPECT_EQ(first.setting.rollDeltaLimit, second.setting.rollDeltaLimit);
    EXPECT_EQ(first.growthRate, second.growthRate);
    EXPECT_EQ(first.medianEquity, second.medianEquity);
    EXPECT_EQ(first.worstDrawdown, second.worstDrawdown);
    EXPECT_EQ(first.assignmentFrequency, second.assignmentFrequency);
}

TEST(TestRecommendation, a_recommended_setting_is_one_the_range_could_have_produced)
{
    const auto recommendation = Unconstrained();

    EXPECT_TRUE(IsWellFormed(recommendation.setting));
    EXPECT_GE(recommendation.setting.targetDelta, SearchedRange().lowest.targetDelta);
    EXPECT_LE(recommendation.setting.targetDelta, SearchedRange().highest.targetDelta);
    EXPECT_GE(recommendation.setting.cashBufferBasisPoints, SearchedRange().lowest.cashBufferBasisPoints);
    EXPECT_LE(recommendation.setting.cashBufferBasisPoints, SearchedRange().highest.cashBufferBasisPoints);
    EXPECT_GE(recommendation.setting.rollDeltaLimit, SearchedRange().lowest.rollDeltaLimit);
    EXPECT_LE(recommendation.setting.rollDeltaLimit, SearchedRange().highest.rollDeltaLimit);
}

TEST(TestRecommendation, a_run_that_reaches_no_limit_is_reported_as_feasible_with_nothing_binding)
{
    const auto recommendation = Unconstrained();

    EXPECT_FALSE(recommendation.drawdownBinds);
    EXPECT_FALSE(recommendation.assignmentBinds);
    EXPECT_TRUE(IsFeasible(recommendation));
}

TEST(TestRecommendation, a_limit_no_setting_can_meet_is_reported_as_binding)
{
    const auto recommendation = Under(RiskLimits{ .maximumDrawdown = 0.0, .maximumAssignmentFrequency = 0.0, .penaltyWeight = 1.0 });

    EXPECT_TRUE(recommendation.drawdownBinds);
    EXPECT_TRUE(recommendation.assignmentBinds);
    EXPECT_FALSE(IsFeasible(recommendation));
}

TEST(TestRecommendation, the_setting_that_grows_fastest_is_the_one_that_commits_the_most_cash)
{
    const auto recommendation = Unconstrained();

    EXPECT_EQ(recommendation.setting.cashBufferBasisPoints, SearchedRange().lowest.cashBufferBasisPoints);
    EXPECT_GT(recommendation.worstDrawdown, drawdownAllowed);
    EXPECT_GT(recommendation.assignmentFrequency, assignmentAllowed);
}

TEST(TestRecommendation, a_drawdown_limit_is_met_by_holding_back_cash_and_is_paid_for_in_growth)
{
    const auto free = Unconstrained();
    const auto held = Under(ShallowDrawdown());

    EXPECT_LE(held.worstDrawdown, drawdownAllowed);
    EXPECT_TRUE(IsFeasible(held));
    EXPECT_GT(held.setting.cashBufferBasisPoints, free.setting.cashBufferBasisPoints);
    EXPECT_LT(held.growthRate, free.growthRate);
}

TEST(TestRecommendation, an_assignment_limit_is_met_by_rolling_sooner_and_is_paid_for_in_growth)
{
    const auto free = Unconstrained();
    const auto held = Under(FewAssignments());

    EXPECT_LE(held.assignmentFrequency, assignmentAllowed);
    EXPECT_TRUE(IsFeasible(held));
    EXPECT_LT(held.setting.rollDeltaLimit, free.setting.rollDeltaLimit);
    EXPECT_LT(held.growthRate, free.growthRate);
}
