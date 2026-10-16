#include "source/optimize/test/StrategyScenario.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    using fin::optimize::BookExperiment;
    using fin::optimize::ExcessOver;
    using fin::optimize::Penalty;
    using fin::optimize::Point;
    using fin::optimize::PointOf;
    using fin::optimize::RiskLimits;
    using fin::optimize::SettingAt;
    using fin::optimize::StrategyObjective;
    using fin::optimize::test::Experiment;
    using fin::optimize::test::Market;
    using fin::optimize::test::SearchedRange;
    using fin::optimize::test::Setting;
    using fin::optimize::test::UnreachableLimits;

    constexpr std::int32_t paths = 24;

    const std::array<Point, 9> box{ Point{ 0.0, 0.0, 0.0 }, Point{ 1.0, 0.0, 0.0 }, Point{ 0.0, 1.0, 0.0 }, Point{ 1.0, 1.0, 0.0 },
        Point{ 0.0, 0.0, 1.0 }, Point{ 1.0, 0.0, 1.0 }, Point{ 0.0, 1.0, 1.0 }, Point{ 1.0, 1.0, 1.0 }, Point{ 0.5, 0.5, 0.5 } };

    StrategyObjective Objective(const RiskLimits& limits)
    {
        return StrategyObjective(Experiment(paths), SearchedRange(), limits);
    }
}

TEST(TestStrategyObjective, an_experiment_that_describes_a_run_is_well_formed)
{
    EXPECT_TRUE(IsWellFormed(Experiment(paths)));
}

TEST(TestStrategyObjective, an_experiment_that_does_not_describe_a_run_is_not_well_formed)
{
    auto nameless = Experiment(paths);
    nameless.terms.underlyings.front().underlying.clear();
    EXPECT_FALSE(IsWellFormed(nameless));

    auto bookless = Experiment(paths);
    bookless.terms.underlyings.clear();
    EXPECT_FALSE(IsWellFormed(bookless));

    auto pathless = Experiment(0);
    EXPECT_FALSE(IsWellFormed(pathless));

    auto unwritable = Experiment(paths);
    unwritable.terms.cycles = 0;
    EXPECT_FALSE(IsWellFormed(unwritable));

    auto unrollable = Experiment(paths);
    unrollable.triggers.profitCaptureBasisPoints = 0;
    EXPECT_FALSE(IsWellFormed(unrollable));
}

TEST(TestStrategyObjective, every_setting_the_range_can_reach_has_a_finite_cost)
{
    const auto objective = Objective(UnreachableLimits());

    for (const auto& at : box)
        EXPECT_TRUE(std::isfinite(objective.Cost(at))) << "at " << at[0] << ", " << at[1] << ", " << at[2];
}

TEST(TestStrategyObjective, the_same_setting_is_scored_the_same_way_every_time_it_is_asked_for)
{
    const auto objective = Objective(UnreachableLimits());
    const Point at{ 0.5, 0.5, 0.5 };

    EXPECT_EQ(objective.Cost(at), objective.Cost(at));
    EXPECT_EQ(objective.GrowthRate(SettingAt(SearchedRange(), at)), objective.GrowthRate(SettingAt(SearchedRange(), at)));
}

TEST(TestStrategyObjective, two_objectives_built_from_the_same_experiment_score_a_setting_alike)
{
    const Point at{ 0.25, 0.75, 0.5 };

    EXPECT_EQ(Objective(UnreachableLimits()).Cost(at), Objective(UnreachableLimits()).Cost(at));
}

TEST(TestStrategyObjective, a_point_outside_the_box_is_scored_as_the_face_it_left_through)
{
    const auto objective = Objective(UnreachableLimits());

    EXPECT_EQ(objective.Cost(Point{ -4.0, 0.5, 9.0 }), objective.Cost(Point{ 0.0, 0.5, 1.0 }));
}

TEST(TestStrategyObjective, the_slope_of_the_score_is_finite_across_the_box)
{
    const auto objective = Objective(UnreachableLimits());

    for (const auto& at : box)
        for (const auto slope : objective.Gradient(at))
            EXPECT_TRUE(std::isfinite(slope)) << "at " << at[0] << ", " << at[1] << ", " << at[2];
}

TEST(TestStrategyObjective, a_run_that_stays_within_every_limit_is_scored_on_its_growth_alone)
{
    const auto objective = Objective(UnreachableLimits());
    const auto setting = Setting(0.30, 2200, 0.60);

    EXPECT_DOUBLE_EQ(objective.Cost(PointOf(SearchedRange(), setting)), -objective.GrowthRate(setting));
}

TEST(TestStrategyObjective, a_run_that_passes_a_limit_is_charged_for_exactly_the_amount_by_which_it_passed)
{
    const RiskLimits strict{ .maximumDrawdown = 0.0, .maximumAssignmentFrequency = 1.0, .penaltyWeight = 1.0 };
    const auto objective = Objective(strict);
    const auto setting = Setting(0.30, 2200, 0.60);
    const auto outcome = objective.Outcome(setting);

    EXPECT_GT(outcome.WorstDrawdown(), 0.0);
    EXPECT_DOUBLE_EQ(objective.Cost(PointOf(SearchedRange(), setting)), Penalty(strict, ExcessOver(strict, outcome)) - outcome.MeanGrowthRate());
}

TEST(TestStrategyObjective, a_book_of_several_underlyings_is_scored_by_the_same_three_choices)
{
    const StrategyObjective objective(Experiment(paths, { Market("ACME", 40.00), Market("BETA", 55.00), Market("GAMMA", 30.00) }), SearchedRange(), UnreachableLimits());
    const Point at{ 0.5, 0.5, 0.5 };

    EXPECT_EQ(objective.Cost(at), objective.Cost(at));

    for (const auto slope : objective.Gradient(at))
        EXPECT_TRUE(std::isfinite(slope));
}

TEST(TestStrategyObjective, a_book_is_not_scored_as_the_underlying_that_happens_to_be_listed_first)
{
    const StrategyObjective one(Experiment(paths, { Market("ACME", 40.00) }), SearchedRange(), UnreachableLimits());
    const StrategyObjective several(Experiment(paths, { Market("ACME", 40.00), Market("BETA", 55.00) }), SearchedRange(), UnreachableLimits());
    const Point at{ 0.5, 0.5, 0.5 };

    EXPECT_NE(one.Cost(at), several.Cost(at));
}
