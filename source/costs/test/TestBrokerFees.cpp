#include "source/costs/BrokerFees.hpp"
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::costs::BrokerFees;
    using fin::costs::FeeSchedule;
    using fin::costs::Trade;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    class TestBrokerFees
        : public ::testing::Test
    {
    protected:
        FeeSchedule schedule{
            .optionFeePerContract = Euro(0.75),
            .shareCommissionPerTrade = Euro(2.00),
            .handlingFeePerTrade = Euro(1.00),
            .connectivityFeeCapPerExchangePerYear = Euro(2.50),
            .connectivityBasisPoints = 25,
            .currencyHandlingBasisPoints = 25
        };

        BrokerFees fees{ schedule };
    };
}

TEST_F(TestBrokerFees, a_schedule_that_is_not_well_formed_is_refused)
{
    auto mixed = schedule;
    mixed.optionFeePerContract = Money::FromCents(75, Currency::Usd);

    EXPECT_DEATH(BrokerFees{ mixed }, "");
}

TEST_F(TestBrokerFees, an_option_trade_costs_the_contract_fee_for_every_contract)
{
    EXPECT_EQ(fees.For(Trade::Options(1)), Euro(0.75));
    EXPECT_EQ(fees.For(Trade::Options(4)), Euro(3.00));
    EXPECT_EQ(fees.For(Trade::Options(40)), Euro(30.00));
}

TEST_F(TestBrokerFees, an_option_trade_pays_no_handling_fee)
{
    EXPECT_EQ(fees.For(Trade::Options(1)), schedule.optionFeePerContract);
}

TEST_F(TestBrokerFees, a_share_trade_costs_the_same_whatever_its_size)
{
    EXPECT_EQ(fees.For(Trade::Shares(1)), Euro(3.00));
    EXPECT_EQ(fees.For(Trade::Shares(100)), Euro(3.00));
    EXPECT_EQ(fees.For(Trade::Shares(10000)), Euro(3.00));
}

TEST_F(TestBrokerFees, a_trade_list_costs_the_sum_of_its_trades)
{
    const std::vector<Trade> trades{
        Trade::Options(1),
        Trade::Options(3),
        Trade::Shares(100),
        Trade::Options(2),
        Trade::Shares(50)
    };

    EXPECT_EQ(fees.For(trades), Euro(10.50));
}

TEST_F(TestBrokerFees, an_empty_trade_list_costs_nothing)
{
    EXPECT_EQ(fees.For(std::vector<Trade>{}), Money::Zero(Currency::Eur));
}

TEST_F(TestBrokerFees, connectivity_below_the_cap_is_the_proportional_amount)
{
    EXPECT_EQ(fees.ConnectivityFor(Euro(900.00), 1), Euro(2.25));
    EXPECT_EQ(fees.ConnectivityFor(Euro(400.00), 1), Euro(1.00));
}

TEST_F(TestBrokerFees, connectivity_is_capped_per_exchange_and_year)
{
    EXPECT_EQ(fees.ConnectivityFor(Euro(1000.00), 1), Euro(2.50));
    EXPECT_EQ(fees.ConnectivityFor(Euro(1000000.00), 1), Euro(2.50));
}

TEST_F(TestBrokerFees, connectivity_is_charged_once_per_exchange)
{
    EXPECT_EQ(fees.ConnectivityFor(Euro(1000.00), 0), Money::Zero(Currency::Eur));
    EXPECT_EQ(fees.ConnectivityFor(Euro(1000.00), 3), Euro(7.50));
    EXPECT_EQ(fees.ConnectivityFor(Euro(400.00), 3), Euro(3.00));
}

TEST_F(TestBrokerFees, connectivity_on_an_empty_portfolio_costs_nothing)
{
    EXPECT_EQ(fees.ConnectivityFor(Money::Zero(Currency::Eur), 5), Money::Zero(Currency::Eur));
}

TEST_F(TestBrokerFees, connectivity_needs_a_portfolio_in_the_schedules_currency)
{
    EXPECT_DEATH(fees.ConnectivityFor(Money::FromCents(100000, Currency::Usd), 1), "");
    EXPECT_DEATH(fees.ConnectivityFor(Euro(-1.00), 1), "");
}
