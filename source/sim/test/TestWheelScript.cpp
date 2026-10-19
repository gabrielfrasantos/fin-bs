#include "source/pricing/Greeks.hpp"
#include "source/sim/WheelScript.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Delta;
    using fin::core::Money;
    using fin::core::OptionType;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::estimation::PriceSeries;
    using fin::pricing::Greeks;
    using fin::pricing::OptionParameters;
    using fin::sim::AnnualReference;
    using fin::sim::WheelScript;
    using fin::sim::WheelTerms;
    using fin::strategy::WheelState;

    constexpr auto targetDelta = 0.30;
    constexpr auto volatility = 0.25;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    WheelTerms Terms()
    {
        return { .start = On(2025, 1, 2),
            .cycles = 6,
            .daysPerCycle = 182,
            .observationDaysBeforeExpiry = 30,
            .targetDelta = Delta::FromValue(targetDelta),
            .strikeIncrement = Euro(1.00),
            .riskFreeRate = Rate::FromContinuous(0.02),
            .dividendYield = Rate::Zero(),
            .dividendWithholdingBasisPoints = 1500,
            .volatility = Volatility::FromAnnualized(volatility),
            .impliedVolatility = Volatility::FromAnnualized(volatility),
            .assessment = AnnualReference{ .month = 1, .day = 1 } };
    }

    PriceSeries FlatPath(const WheelScript& script, double price)
    {
        const auto schedule = script.Schedule();

        return PriceSeries::From(schedule, std::vector<Money>(schedule.size(), Euro(price)), DayCountConvention::Act365Fixed);
    }

    double MagnitudeOfDelta(const Money& spot, const Money& strike, OptionType type, double timeToExpiry)
    {
        const OptionParameters parameters{ .spot = spot,
            .strike = strike,
            .riskFreeRate = Rate::FromContinuous(0.02),
            .dividendYield = Rate::Zero(),
            .timeToExpiry = timeToExpiry,
            .type = type };

        return Greeks(parameters, Volatility::FromAnnualized(volatility)).Delta().Magnitude();
    }

    class TestWheelScript
        : public ::testing::Test
    {
    protected:
        WheelScript script{ Terms() };
        PriceSeries path{ FlatPath(script, 40.00) };
        double timeToExpiry{ fin::core::YearFraction(On(2025, 1, 2), On(2025, 7, 3), DayCountConvention::Act365Fixed) };
    };
}

TEST_F(TestWheelScript, the_schedule_runs_from_the_opening_date_to_the_last_expiry)
{
    const auto schedule = script.Schedule();

    EXPECT_EQ(schedule.front(), On(2025, 1, 2));
    EXPECT_EQ(schedule.back(), On(2025, 1, 2).AddDays(6 * 182));
}

TEST_F(TestWheelScript, the_schedule_is_strictly_increasing_and_free_of_repeats)
{
    const auto schedule = script.Schedule();

    for (std::size_t date = 1; date != schedule.size(); ++date)
        EXPECT_LT(schedule[date - 1], schedule[date]);
}

TEST_F(TestWheelScript, every_cycle_boundary_observation_and_reference_date_is_priced)
{
    for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
    {
        const auto period = script.PeriodFor(cycle, WheelState::Cash, path);

        EXPECT_EQ(period.opened, On(2025, 1, 2).AddDays(cycle * 182));
        EXPECT_EQ(period.expiry, period.opened.AddDays(182));
        ASSERT_TRUE(period.observed.has_value());
        EXPECT_EQ(period.observed->on, period.expiry.AddDays(-30));
    }
}

TEST_F(TestWheelScript, a_wheel_holding_cash_writes_a_put_below_the_spot)
{
    const auto period = script.PeriodFor(0, WheelState::Cash, path);

    EXPECT_LT(period.strike, Euro(40.00));
    EXPECT_GT(period.premiumPerShare, 0.0);
}

TEST_F(TestWheelScript, a_wheel_holding_shares_writes_a_call_above_the_spot)
{
    const auto period = script.PeriodFor(0, WheelState::HoldingShares, path);

    EXPECT_GT(period.strike, Euro(40.00));
    EXPECT_GT(period.premiumPerShare, 0.0);
}

TEST_F(TestWheelScript, a_covered_call_is_never_offered_a_roll)
{
    for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
        EXPECT_FALSE(script.PeriodFor(cycle, WheelState::HoldingShares, path).observed.has_value());
}

TEST_F(TestWheelScript, the_put_is_the_nearest_tradable_strike_whose_delta_does_not_exceed_the_target)
{
    const auto strike = script.PeriodFor(0, WheelState::Cash, path).strike;

    EXPECT_LE(MagnitudeOfDelta(Euro(40.00), strike, OptionType::Put, timeToExpiry), targetDelta);
    EXPECT_GT(MagnitudeOfDelta(Euro(40.00), strike + Euro(1.00), OptionType::Put, timeToExpiry), targetDelta);
}

TEST_F(TestWheelScript, the_call_is_the_nearest_tradable_strike_whose_delta_does_not_exceed_the_target)
{
    const auto strike = script.PeriodFor(0, WheelState::HoldingShares, path).strike;

    EXPECT_LE(MagnitudeOfDelta(Euro(40.00), strike, OptionType::Call, timeToExpiry), targetDelta);
    EXPECT_GT(MagnitudeOfDelta(Euro(40.00), strike - Euro(1.00), OptionType::Call, timeToExpiry), targetDelta);
}

TEST_F(TestWheelScript, the_option_is_worth_less_at_the_observation_than_it_was_written_for)
{
    const auto period = script.PeriodFor(0, WheelState::Cash, path);

    ASSERT_TRUE(period.observed.has_value());
    EXPECT_LT(period.observed->valuePerShare, period.premiumPerShare);
    EXPECT_GT(period.observed->valuePerShare, 0.0);
    EXPECT_LT(period.observed->delta.Value(), 0.0);
}

TEST_F(TestWheelScript, the_expiry_carries_the_price_the_path_reached)
{
    const auto rising = PriceSeries::From(script.Schedule(), std::vector<Money>(script.Schedule().size(), Euro(55.00)), DayCountConvention::Act365Fixed);

    EXPECT_EQ(script.PeriodFor(0, WheelState::Cash, rising).spotAtExpiry, Euro(55.00));
}

TEST_F(TestWheelScript, a_reference_date_is_carried_by_the_one_period_that_contains_it)
{
    std::vector<Date> assessed;

    for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
        if (const auto period = script.PeriodFor(cycle, WheelState::Cash, path); period.assessed.has_value())
            assessed.push_back(period.assessed->on);

    EXPECT_EQ(assessed, (std::vector<Date>{ On(2026, 1, 1), On(2027, 1, 1) }));
}

TEST_F(TestWheelScript, a_script_that_is_never_assessed_carries_no_reference_date)
{
    auto terms = Terms();
    terms.assessment.reset();

    const WheelScript untaxed(terms);

    for (std::int32_t cycle = 0; cycle != untaxed.Cycles(); ++cycle)
        EXPECT_FALSE(untaxed.PeriodFor(cycle, WheelState::Cash, FlatPath(untaxed, 40.00)).assessed.has_value());
}

TEST_F(TestWheelScript, the_horizon_is_the_span_from_the_opening_date_to_the_last_expiry)
{
    EXPECT_DOUBLE_EQ(script.Horizon(), 6.0 * 182.0 / 365.0);
}

TEST_F(TestWheelScript, terms_are_refused_when_the_observation_is_not_inside_the_cycle)
{
    auto terms = Terms();
    terms.observationDaysBeforeExpiry = terms.daysPerCycle;

    EXPECT_FALSE(IsWellFormed(terms));
}

TEST_F(TestWheelScript, terms_are_refused_when_the_reference_date_does_not_exist_in_every_year)
{
    auto terms = Terms();
    terms.assessment = AnnualReference{ .month = 2, .day = 29 };

    EXPECT_FALSE(IsWellFormed(terms));
}

TEST_F(TestWheelScript, terms_are_refused_when_the_target_delta_leaves_no_strike_to_choose)
{
    auto terms = Terms();
    terms.targetDelta = Delta::FromValue(1.0);

    EXPECT_FALSE(IsWellFormed(terms));
}

TEST_F(TestWheelScript, the_premium_is_priced_at_the_implied_volatility_and_not_at_the_one_the_path_moves_with)
{
    auto terms = Terms();
    terms.impliedVolatility = Volatility::FromAnnualized(volatility + 0.10);
    const WheelScript richer(terms);

    const auto fair = script.PeriodFor(0, WheelState::Cash, path);
    const auto rich = richer.PeriodFor(0, WheelState::Cash, FlatPath(richer, 40.00));

    EXPECT_GT(rich.premiumPerShare, fair.premiumPerShare);
    EXPECT_LE(rich.strike, fair.strike);
}

TEST_F(TestWheelScript, a_period_carries_the_dividend_the_yield_accrues_over_the_cycle_and_the_rate_withheld_from_it)
{
    auto terms = Terms();
    terms.dividendYield = Rate::FromContinuous(0.04);
    const WheelScript paying(terms);

    const auto period = paying.PeriodFor(0, WheelState::HoldingShares, FlatPath(paying, 40.00));

    EXPECT_DOUBLE_EQ(period.dividendPerShare, 40.00 * std::expm1(0.04 * timeToExpiry));
    EXPECT_EQ(period.dividendWithholdingBasisPoints, 1500);
    EXPECT_DOUBLE_EQ(script.PeriodFor(0, WheelState::HoldingShares, path).dividendPerShare, 0.0);
}

TEST_F(TestWheelScript, terms_are_refused_without_an_implied_volatility_or_with_a_withholding_beyond_the_whole)
{
    auto unpriced = Terms();
    unpriced.impliedVolatility = Volatility::FromAnnualized(0.0);

    auto overWithheld = Terms();
    overWithheld.dividendWithholdingBasisPoints = 10001;

    EXPECT_FALSE(IsWellFormed(unpriced));
    EXPECT_FALSE(IsWellFormed(overWithheld));
}
