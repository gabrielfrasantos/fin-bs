#include "source/sim/OutcomeDistribution.hpp"
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::sim::OutcomeDistribution;
    using fin::sim::PathOutcome;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    PathOutcome Ending(double equity, double growthRate, std::int32_t assignments, double drawdown)
    {
        return { .terminalEquity = Euro(equity), .growthRate = growthRate, .positions = 4, .assignments = assignments, .maximumDrawdown = drawdown };
    }

    OutcomeDistribution HundredOutcomes()
    {
        std::vector<PathOutcome> outcomes;

        for (std::int32_t rank = 100; rank != 0; --rank)
            outcomes.push_back(Ending(rank, rank / 100.0, rank % 5, rank / 1000.0));

        return OutcomeDistribution::Of(std::move(outcomes));
    }
}

TEST(TestOutcomeDistribution, a_percentile_is_the_outcome_at_that_rank_and_not_an_interpolation)
{
    const auto distribution = HundredOutcomes();

    EXPECT_EQ(distribution.Paths(), 100u);
    EXPECT_EQ(distribution.Equity(5), Euro(5.00));
    EXPECT_EQ(distribution.Equity(50), Euro(50.00));
    EXPECT_EQ(distribution.Equity(95), Euro(95.00));
    EXPECT_EQ(distribution.Equity(100), Euro(100.00));
}

TEST(TestOutcomeDistribution, the_percentiles_are_ordered)
{
    const auto distribution = HundredOutcomes();

    EXPECT_LE(distribution.Equity(5), distribution.Equity(50));
    EXPECT_LE(distribution.Equity(50), distribution.Equity(95));
    EXPECT_LE(distribution.GrowthRate(5), distribution.GrowthRate(50));
    EXPECT_LE(distribution.GrowthRate(50), distribution.GrowthRate(95));
}

TEST(TestOutcomeDistribution, the_growth_rate_reported_at_a_percentile_belongs_to_the_path_ranked_there)
{
    const auto distribution = HundredOutcomes();

    EXPECT_DOUBLE_EQ(distribution.GrowthRate(50), 0.50);
    EXPECT_EQ(distribution.At(50).terminalEquity, distribution.Equity(50));
}

TEST(TestOutcomeDistribution, a_rank_that_falls_between_two_paths_is_rounded_up_to_the_next_one)
{
    const auto distribution = OutcomeDistribution::Of({ Ending(10.00, 0.1, 0, 0.0), Ending(20.00, 0.2, 0, 0.0), Ending(30.00, 0.3, 0, 0.0) });

    EXPECT_EQ(distribution.Equity(1), Euro(10.00));
    EXPECT_EQ(distribution.Equity(33), Euro(10.00));
    EXPECT_EQ(distribution.Equity(34), Euro(20.00));
    EXPECT_EQ(distribution.Equity(67), Euro(30.00));
}

TEST(TestOutcomeDistribution, a_single_path_is_its_own_every_percentile)
{
    const auto distribution = OutcomeDistribution::Of({ Ending(1234.56, 0.07, 2, 0.25) });

    EXPECT_EQ(distribution.Equity(5), Euro(1234.56));
    EXPECT_EQ(distribution.Equity(50), Euro(1234.56));
    EXPECT_EQ(distribution.Equity(95), Euro(1234.56));
}

TEST(TestOutcomeDistribution, the_means_average_every_path)
{
    const auto distribution = OutcomeDistribution::Of({ Ending(10.00, 0.10, 0, 0.0), Ending(20.00, 0.20, 0, 0.0), Ending(31.00, 0.30, 0, 0.0) });

    EXPECT_EQ(distribution.MeanEquity(), Euro(20.33));
    EXPECT_DOUBLE_EQ(distribution.MeanGrowthRate(), 0.20);
}

TEST(TestOutcomeDistribution, the_worst_drawdown_is_the_worst_any_path_suffered)
{
    const auto distribution = OutcomeDistribution::Of({ Ending(10.00, 0.1, 0, 0.15), Ending(20.00, 0.2, 0, 0.40), Ending(30.00, 0.3, 0, 0.05) });

    EXPECT_DOUBLE_EQ(distribution.WorstDrawdown(), 0.40);
}

TEST(TestOutcomeDistribution, the_assignment_frequency_counts_assignments_against_every_cycle_run)
{
    const auto distribution = OutcomeDistribution::Of({ Ending(10.00, 0.1, 1, 0.0), Ending(20.00, 0.2, 3, 0.0), Ending(30.00, 0.3, 2, 0.0) });

    EXPECT_DOUBLE_EQ(distribution.AssignmentFrequency(), 6.0 / 12.0);
}
