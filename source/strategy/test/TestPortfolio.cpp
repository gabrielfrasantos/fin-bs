#include "source/strategy/Portfolio.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::core::OptionContract;
    using fin::core::OptionType;
    using fin::strategy::Equity;
    using fin::strategy::Portfolio;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date Expiry()
    {
        return Date::FromYearMonthDay(2025, 6, 20);
    }

    OptionContract Put(double strike)
    {
        return OptionContract("ACME", Expiry(), Euro(strike), OptionType::Put);
    }

    OptionContract Call(double strike)
    {
        return OptionContract("ACME", Expiry(), Euro(strike), OptionType::Call);
    }

    class TestPortfolio
        : public ::testing::Test
    {
    protected:
        Portfolio portfolio{ Portfolio::Funded(Euro(20000.00)) };
    };
}

TEST_F(TestPortfolio, a_funded_portfolio_holds_nothing_but_cash)
{
    EXPECT_EQ(portfolio.Cash(), Euro(20000.00));
    EXPECT_EQ(portfolio.FreeCash(), Euro(20000.00));
    EXPECT_EQ(portfolio.Collateral(), Euro(0.00));
    EXPECT_EQ(portfolio.Shares(), 0);
    EXPECT_TRUE(portfolio.Lots().empty());
    EXPECT_TRUE(portfolio.ShortPositions().empty());
}

TEST_F(TestPortfolio, a_portfolio_cannot_be_funded_with_a_debt)
{
    EXPECT_DEATH(Portfolio::Funded(Euro(-0.01)), "");
}

TEST_F(TestPortfolio, selling_a_put_reserves_the_strike_for_every_share_it_may_have_to_buy)
{
    portfolio.SellPut(Put(40.00), 2);

    EXPECT_EQ(portfolio.Collateral(), Euro(8000.00));
    EXPECT_EQ(portfolio.FreeCash(), Euro(12000.00));
    EXPECT_EQ(portfolio.Cash(), Euro(20000.00));
    EXPECT_EQ(portfolio.ContractsShortOf(Put(40.00)), 2);
}

TEST_F(TestPortfolio, a_put_that_the_cash_cannot_secure_is_refused)
{
    EXPECT_DEATH(portfolio.SellPut(Put(40.00), 6), "");
}

TEST_F(TestPortfolio, selling_the_same_contract_again_adds_to_the_position)
{
    portfolio.SellPut(Put(40.00), 1);
    portfolio.SellPut(Put(40.00), 2);

    EXPECT_EQ(portfolio.ShortPositions().size(), 1u);
    EXPECT_EQ(portfolio.ContractsShortOf(Put(40.00)), 3);
    EXPECT_EQ(portfolio.Collateral(), Euro(12000.00));
}

TEST_F(TestPortfolio, a_call_is_only_written_against_shares_that_nothing_else_covers)
{
    portfolio.SellPut(Put(40.00), 1);
    portfolio.Assign(Put(40.00));

    EXPECT_EQ(portfolio.UncoveredShares(), 100);

    portfolio.SellCall(Call(45.00), 1);

    EXPECT_EQ(portfolio.CoveredShares(), 100);
    EXPECT_EQ(portfolio.UncoveredShares(), 0);
    EXPECT_EQ(portfolio.Collateral(), Euro(0.00));
    EXPECT_DEATH(portfolio.SellCall(Call(50.00), 1), "");
}

TEST_F(TestPortfolio, money_that_is_reserved_cannot_also_be_spent)
{
    portfolio.SellPut(Put(40.00), 4);

    EXPECT_EQ(portfolio.FreeCash(), Euro(4000.00));

    portfolio.Pay(Euro(4000.00));

    EXPECT_EQ(portfolio.FreeCash(), Euro(0.00));
    EXPECT_DEATH(portfolio.Pay(Euro(0.01)), "");
}

TEST_F(TestPortfolio, a_negative_receipt_is_not_a_way_of_paying)
{
    EXPECT_DEATH(portfolio.Receive(Euro(-1.00)), "");
    EXPECT_DEATH(portfolio.Pay(Euro(-1.00)), "");
}

TEST_F(TestPortfolio, an_amount_in_another_currency_is_refused)
{
    EXPECT_DEATH(portfolio.Receive(Money::FromAmount(1.00, Currency::Usd)), "");
}

TEST_F(TestPortfolio, being_assigned_on_a_put_turns_the_reserved_cash_into_shares)
{
    portfolio.SellPut(Put(40.00), 2);
    portfolio.Assign(Put(40.00));

    EXPECT_EQ(portfolio.Cash(), Euro(12000.00));
    EXPECT_EQ(portfolio.Collateral(), Euro(0.00));
    EXPECT_EQ(portfolio.Shares(), 200);
    EXPECT_EQ(portfolio.Lots().front().pricePerShare, Euro(40.00));
    EXPECT_TRUE(portfolio.ShortPositions().empty());
}

TEST_F(TestPortfolio, being_assigned_on_a_call_turns_the_shares_back_into_cash)
{
    portfolio.SellPut(Put(40.00), 2);
    portfolio.Assign(Put(40.00));
    portfolio.SellCall(Call(45.00), 2);
    portfolio.Assign(Call(45.00));

    EXPECT_EQ(portfolio.Cash(), Euro(21000.00));
    EXPECT_EQ(portfolio.Shares(), 0);
    EXPECT_TRUE(portfolio.Lots().empty());
}

TEST_F(TestPortfolio, assignment_moves_value_between_cash_and_shares_without_creating_any)
{
    const auto spot = Euro(36.50);

    portfolio.SellPut(Put(40.00), 2);

    const auto before = Equity(portfolio, spot);

    portfolio.Assign(Put(40.00));

    EXPECT_EQ(Equity(portfolio, spot), before);
}

TEST_F(TestPortfolio, shares_leave_the_portfolio_oldest_lot_first)
{
    portfolio.SellPut(Put(40.00), 1);
    portfolio.Assign(Put(40.00));
    portfolio.SellPut(Put(50.00), 1);
    portfolio.Assign(Put(50.00));

    EXPECT_EQ(portfolio.Lots().size(), 2u);

    portfolio.SellCall(Call(60.00), 1);
    portfolio.Assign(Call(60.00));

    ASSERT_EQ(portfolio.Lots().size(), 1u);
    EXPECT_EQ(portfolio.Lots().front().pricePerShare, Euro(50.00));
    EXPECT_EQ(portfolio.Shares(), 100);
}

TEST_F(TestPortfolio, closing_a_position_releases_its_collateral_without_moving_any_cash)
{
    portfolio.SellPut(Put(40.00), 2);
    portfolio.Close(Put(40.00));

    EXPECT_EQ(portfolio.Collateral(), Euro(0.00));
    EXPECT_EQ(portfolio.Cash(), Euro(20000.00));
    EXPECT_TRUE(portfolio.ShortPositions().empty());
}

TEST_F(TestPortfolio, a_position_that_was_never_opened_cannot_be_closed_or_assigned)
{
    EXPECT_DEATH(portfolio.Close(Put(40.00)), "");
    EXPECT_DEATH(portfolio.Assign(Put(40.00)), "");
}

TEST_F(TestPortfolio, equity_marks_the_shares_at_what_they_are_worth_now)
{
    portfolio.SellPut(Put(40.00), 1);
    portfolio.Assign(Put(40.00));

    EXPECT_EQ(Equity(portfolio, Euro(40.00)), Euro(20000.00));
    EXPECT_EQ(Equity(portfolio, Euro(45.00)), Euro(20500.00));
    EXPECT_EQ(Equity(portfolio, Euro(35.00)), Euro(19500.00));
}

TEST_F(TestPortfolio, equity_carries_a_short_position_at_what_it_would_cost_to_settle)
{
    portfolio.SellPut(Put(40.00), 2);

    EXPECT_EQ(Equity(portfolio, Euro(45.00)), Euro(20000.00));
    EXPECT_EQ(Equity(portfolio, Euro(38.00)), Euro(19600.00));
}
