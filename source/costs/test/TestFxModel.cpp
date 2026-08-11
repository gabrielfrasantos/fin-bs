#include "source/costs/FxModel.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::costs::FxModel;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Money Dollar(double amount)
    {
        return Money::FromAmount(amount, Currency::Usd);
    }

    class TestFxModel
        : public ::testing::Test
    {
    protected:
        static constexpr double dollarsPerEuro = 1.10;
        static constexpr std::int32_t autoFxBasisPoints = 25;

        FxModel model{ FxModel::From(Currency::Usd, dollarsPerEuro, autoFxBasisPoints) };
        FxModel free{ FxModel::From(Currency::Usd, dollarsPerEuro, 0) };
    };
}

TEST_F(TestFxModel, the_base_currency_is_the_one_the_project_reports_in)
{
    EXPECT_EQ(model.Base(), Currency::Eur);
    EXPECT_EQ(model.Foreign(), Currency::Usd);
}

TEST_F(TestFxModel, a_rate_that_is_not_a_rate_is_refused)
{
    EXPECT_DEATH(FxModel::From(Currency::Eur, dollarsPerEuro, autoFxBasisPoints), "");
    EXPECT_DEATH(FxModel::From(Currency::Usd, 0.0, autoFxBasisPoints), "");
    EXPECT_DEATH(FxModel::From(Currency::Usd, -1.10, autoFxBasisPoints), "");
}

TEST_F(TestFxModel, a_handling_cost_of_the_whole_amount_is_refused)
{
    EXPECT_DEATH(FxModel::From(Currency::Usd, dollarsPerEuro, -1), "");
    EXPECT_DEATH(FxModel::From(Currency::Usd, dollarsPerEuro, 10000), "");
}

TEST_F(TestFxModel, converting_at_the_spot_rate_changes_the_currency)
{
    EXPECT_EQ(model.AtSpot(Euro(100.00)), Dollar(110.00));
    EXPECT_EQ(model.AtSpot(Dollar(110.00)), Euro(100.00));
}

TEST_F(TestFxModel, the_handling_cost_is_charged_on_the_size_of_the_amount_not_its_direction)
{
    EXPECT_EQ(model.HandlingCost(Dollar(400.00)), Dollar(1.00));
    EXPECT_EQ(model.HandlingCost(Dollar(-400.00)), Dollar(1.00));
    EXPECT_EQ(model.HandlingCost(Euro(0.00)), Euro(0.00));
}

TEST_F(TestFxModel, a_conversion_is_the_spot_amount_less_the_handling_cost)
{
    const auto amount = Euro(100.00);
    const auto spot = model.AtSpot(amount);

    EXPECT_EQ(model.Convert(amount), spot - model.HandlingCost(spot));
}

TEST_F(TestFxModel, an_outgoing_cashflow_costs_more_than_the_spot_amount)
{
    const auto payment = Euro(-1000.00);

    EXPECT_LT(model.Convert(payment), model.AtSpot(payment));
}

TEST_F(TestFxModel, an_incoming_cashflow_arrives_short_of_the_spot_amount)
{
    const auto receipt = Euro(1000.00);

    EXPECT_LT(model.Convert(receipt), model.AtSpot(receipt));
}

TEST_F(TestFxModel, a_round_trip_loses_twice_the_handling_cost_less_its_square)
{
    const auto start = Euro(10000.00);
    const auto returned = model.Convert(model.Convert(start));

    const auto handling = autoFxBasisPoints / 10000.0;
    const auto expectedLoss = start.ToDouble() * (1.0 - (1.0 - handling) * (1.0 - handling));

    EXPECT_NEAR((start - returned).ToDouble(), expectedLoss, 0.01);
}

TEST_F(TestFxModel, a_round_trip_without_a_handling_cost_returns_what_it_started_with)
{
    const auto start = Euro(10000.00);

    EXPECT_NEAR(free.Convert(free.Convert(start)).ToDouble(), start.ToDouble(), 0.01);
}

TEST_F(TestFxModel, a_round_trip_never_returns_more_than_it_started_with)
{
    const auto start = Dollar(5000.00);

    EXPECT_LE(model.Convert(model.Convert(start)), start);
}
