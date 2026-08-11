#include "source/pricing/AssignmentModel.hpp"
#include "source/pricing/test/OptionScenario.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::core::OptionContract;
    using fin::core::OptionType;
    using fin::core::Volatility;
    using fin::pricing::AssignmentModel;
    using fin::pricing::AssignmentOutcome;
    using fin::pricing::test::OptionScenario;

    class TestAssignmentModel
        : public ::testing::Test
    {
    protected:
        static OptionContract Contract(OptionType type)
        {
            return OptionContract("ASML", Date::FromYearMonthDay(2025, 3, 21), Money::FromAmount(100.0, Currency::Eur), type);
        }

        static AssignmentOutcome AtExpiry(OptionType type, double spot)
        {
            return AssignmentModel::AtExpiry(Contract(type), Money::FromAmount(spot, Currency::Eur));
        }

        static AssignmentOutcome BeforeExDividend(double spot, double dividendPerShare)
        {
            return AssignmentModel::BeforeExDividend(OptionScenario(spot, 100.0, 0.03, 0.0, 0.02, OptionType::Call), Volatility::FromAnnualized(0.2), Money::FromAmount(dividendPerShare, Currency::Eur));
        }
    };
}

TEST_F(TestAssignmentModel, a_call_is_assigned_at_expiry_only_above_its_strike)
{
    EXPECT_EQ(AtExpiry(OptionType::Call, 100.01), AssignmentOutcome::Assigned);
    EXPECT_EQ(AtExpiry(OptionType::Call, 140.0), AssignmentOutcome::Assigned);
    EXPECT_EQ(AtExpiry(OptionType::Call, 100.0), AssignmentOutcome::NotAssigned);
    EXPECT_EQ(AtExpiry(OptionType::Call, 99.99), AssignmentOutcome::NotAssigned);
}

TEST_F(TestAssignmentModel, a_put_is_assigned_at_expiry_only_below_its_strike)
{
    EXPECT_EQ(AtExpiry(OptionType::Put, 99.99), AssignmentOutcome::Assigned);
    EXPECT_EQ(AtExpiry(OptionType::Put, 60.0), AssignmentOutcome::Assigned);
    EXPECT_EQ(AtExpiry(OptionType::Put, 100.0), AssignmentOutcome::NotAssigned);
    EXPECT_EQ(AtExpiry(OptionType::Put, 100.01), AssignmentOutcome::NotAssigned);
}

TEST_F(TestAssignmentModel, an_in_the_money_call_is_assigned_early_when_the_dividend_exceeds_its_remaining_time_value)
{
    EXPECT_EQ(BeforeExDividend(120.0, 0.5), AssignmentOutcome::Assigned);
    EXPECT_EQ(BeforeExDividend(120.0, 0.01), AssignmentOutcome::NotAssigned);
}

TEST_F(TestAssignmentModel, an_out_of_the_money_call_is_never_assigned_early)
{
    EXPECT_EQ(BeforeExDividend(80.0, 25.0), AssignmentOutcome::NotAssigned);
    EXPECT_EQ(BeforeExDividend(100.0, 25.0), AssignmentOutcome::NotAssigned);
}

TEST_F(TestAssignmentModel, time_value_grows_with_the_horizon_so_a_longer_dated_call_is_held)
{
    const auto volatility = Volatility::FromAnnualized(0.2);
    const auto dividend = Money::FromAmount(0.5, Currency::Eur);

    EXPECT_EQ(AssignmentModel::BeforeExDividend(OptionScenario(120.0, 100.0, 0.03, 0.0, 0.02, OptionType::Call), volatility, dividend), AssignmentOutcome::Assigned);
    EXPECT_EQ(AssignmentModel::BeforeExDividend(OptionScenario(120.0, 100.0, 0.03, 0.0, 1.0, OptionType::Call), volatility, dividend), AssignmentOutcome::NotAssigned);
}

TEST_F(TestAssignmentModel, refuses_an_early_assignment_question_that_cannot_be_answered)
{
    const auto parameters = OptionScenario(120.0, 100.0, 0.03, 0.0, 0.02, OptionType::Call);
    const auto volatility = Volatility::FromAnnualized(0.2);

    EXPECT_DEATH(static_cast<void>(AssignmentModel::BeforeExDividend(OptionScenario(120.0, 100.0, 0.03, 0.0, 0.02, OptionType::Put), volatility, Money::FromAmount(0.5, Currency::Eur))), "");
    EXPECT_DEATH(static_cast<void>(AssignmentModel::BeforeExDividend(parameters, volatility, Money::FromAmount(0.5, Currency::Usd))), "");
    EXPECT_DEATH(static_cast<void>(AssignmentModel::BeforeExDividend(parameters, volatility, Money::FromAmount(-0.5, Currency::Eur))), "");
}
