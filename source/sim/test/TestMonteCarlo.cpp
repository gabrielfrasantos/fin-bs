#include "source/sim/MonteCarlo.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <span>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Delta;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::costs::BrokerFees;
    using fin::costs::FeeSchedule;
    using fin::sim::AnnualReference;
    using fin::sim::GbmParameters;
    using fin::sim::GbmPathGenerator;
    using fin::sim::MonteCarlo;
    using fin::sim::Rng;
    using fin::sim::WheelScript;
    using fin::sim::WheelTerms;
    using fin::strategy::Portfolio;
    using fin::strategy::Projection;
    using fin::strategy::ProjectionPeriod;
    using fin::strategy::ReinvestmentPolicy;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;
    using fin::tax::Box3Parameters;
    using fin::tax::Box3TaxModel;

    constexpr std::uint64_t seed = 20250811u;
    constexpr auto capital = 200000.00;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    BrokerFees Fees()
    {
        return BrokerFees(FeeSchedule{ .optionFeePerContract = Euro(0.75),
            .shareCommissionPerTrade = Euro(2.00),
            .handlingFeePerTrade = Euro(1.00),
            .connectivityFeeCapPerExchangePerYear = Euro(2.50),
            .connectivityBasisPoints = 25,
            .currencyHandlingBasisPoints = 25 });
    }

    Box3TaxModel Rules()
    {
        return Box3TaxModel(Box3Parameters{ .taxYear = 2025,
            .taxFreeAllowance = Euro(57684.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 });
    }

    Box3TaxModel RulesThatChargeNothing()
    {
        return Box3TaxModel(Box3Parameters{ .taxYear = 2025,
            .taxFreeAllowance = Euro(100000000.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 });
    }

    Projection Wheel()
    {
        return Projection("ACME", Portfolio::Funded(Euro(capital)), ReinvestmentPolicy::WithCashBuffer(500), RollRules(RollTriggers{ .daysToExpiry = 7, .profitCaptureBasisPoints = 8000, .deltaLimit = Delta::FromValue(0.60) }), Fees(), Rules());
    }

    Projection UntaxedWheel()
    {
        return Projection("ACME", Portfolio::Funded(Euro(capital)), ReinvestmentPolicy::WithCashBuffer(500), RollRules(RollTriggers{ .daysToExpiry = 7, .profitCaptureBasisPoints = 8000, .deltaLimit = Delta::FromValue(0.60) }), Fees(), RulesThatChargeNothing());
    }

    WheelTerms Terms()
    {
        return { .start = On(2025, 1, 2),
            .cycles = 6,
            .daysPerCycle = 182,
            .observationDaysBeforeExpiry = 30,
            .targetDelta = Delta::FromValue(0.30),
            .strikeIncrement = Euro(1.00),
            .riskFreeRate = Rate::FromContinuous(0.02),
            .dividendYield = Rate::Zero(),
            .volatility = Volatility::FromAnnualized(0.25),
            .assessment = AnnualReference{ .month = 1, .day = 1 } };
    }

    GbmPathGenerator Underlying()
    {
        return GbmPathGenerator(GbmParameters{ .initialPrice = Euro(40.00),
            .drift = Rate::FromContinuous(0.07),
            .volatility = Volatility::FromAnnualized(0.25),
            .convention = DayCountConvention::Act365Fixed });
    }

    class TestMonteCarlo
        : public ::testing::Test
    {
    protected:
        WheelScript script{ Terms() };
        GbmPathGenerator underlying{ Underlying() };
        MonteCarlo simulation{ Wheel(), script, underlying };
    };
}

TEST_F(TestMonteCarlo, one_path_is_the_projection_of_the_script_that_path_writes)
{
    Rng own(seed);
    const auto path = underlying.Along(script.Schedule(), own);
    auto projection = Wheel();
    std::vector<fin::strategy::ProjectionRow> rows;

    for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
    {
        const ProjectionPeriod period = script.PeriodFor(cycle, projection.State(), path);

        for (const auto& row : projection.Over(std::span<const ProjectionPeriod>(&period, 1)))
            rows.push_back(row);
    }

    Rng simulated(seed);
    const auto distribution = simulation.Over(1, simulated);

    ASSERT_EQ(distribution.Paths(), 1u);
    EXPECT_EQ(distribution.Equity(100), rows.back().equity);
    EXPECT_EQ(distribution.At(100).positions, script.Cycles());
}

TEST_F(TestMonteCarlo, the_same_seed_gives_the_same_distribution)
{
    Rng first(seed);
    Rng second(seed);

    EXPECT_EQ(simulation.Over(32, first).Equity(50), simulation.Over(32, second).Equity(50));
}

TEST_F(TestMonteCarlo, a_different_seed_gives_a_different_distribution)
{
    Rng first(seed);
    Rng second(seed + 1u);

    EXPECT_NE(simulation.Over(32, first).Equity(50), simulation.Over(32, second).Equity(50));
}

TEST_F(TestMonteCarlo, no_path_runs_out_of_the_cash_it_needs_to_settle_what_it_owes)
{
    for (std::uint64_t stress = 0; stress != 40u; ++stress)
    {
        Rng rng(seed + stress);
        const auto distribution = simulation.Over(256, rng);

        ASSERT_EQ(distribution.Paths(), 256u);
        EXPECT_EQ(distribution.At(50).positions, script.Cycles());
        EXPECT_GE(distribution.AssignmentFrequency(), 0.0);
        EXPECT_LE(distribution.AssignmentFrequency(), 1.0);
        EXPECT_GE(distribution.WorstDrawdown(), 0.0);
        EXPECT_LT(distribution.WorstDrawdown(), 1.0);
    }
}

TEST_F(TestMonteCarlo, the_outcomes_spread_around_the_median)
{
    Rng rng(seed);
    const auto distribution = simulation.Over(64, rng);

    EXPECT_LT(distribution.Equity(5), distribution.Equity(50));
    EXPECT_LT(distribution.Equity(50), distribution.Equity(95));
    EXPECT_LT(distribution.GrowthRate(5), distribution.GrowthRate(95));
}

TEST_F(TestMonteCarlo, the_median_growth_rate_is_the_growth_of_the_median_terminal_equity)
{
    Rng rng(seed);
    const auto distribution = simulation.Over(64, rng);
    const auto grown = std::pow(distribution.Equity(50).ToDouble() / capital, 1.0 / script.Horizon()) - 1.0;

    EXPECT_DOUBLE_EQ(distribution.GrowthRate(50), grown);
}

TEST_F(TestMonteCarlo, an_assessed_wheel_ends_below_the_same_wheel_left_untaxed)
{
    const MonteCarlo exempt(UntaxedWheel(), script, underlying);

    Rng assessed(seed);
    Rng untaxed(seed);

    EXPECT_LT(simulation.Over(16, assessed).MeanEquity(), exempt.Over(16, untaxed).MeanEquity());
}
