#include "source/strategy/Portfolio.hpp"
#include <array>
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
    using fin::strategy::SpotPrice;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date Expiry()
    {
        return Date::FromYearMonthDay(2025, 6, 20);
    }

    OptionContract Put(const std::string& underlying, double strike)
    {
        return OptionContract(underlying, Expiry(), Euro(strike), OptionType::Put);
    }

    OptionContract Call(const std::string& underlying, double strike)
    {
        return OptionContract(underlying, Expiry(), Euro(strike), OptionType::Call);
    }

    OptionContract Put(double strike)
    {
        return Put("ACME", strike);
    }

    OptionContract Call(double strike)
    {
        return Call("ACME", strike);
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
    EXPECT_EQ(portfolio.Shares("ACME"), 0);
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

    EXPECT_EQ(portfolio.UncoveredShares("ACME"), 100);

    portfolio.SellCall(Call(45.00), 1);

    EXPECT_EQ(portfolio.CoveredShares("ACME"), 100);
    EXPECT_EQ(portfolio.UncoveredShares("ACME"), 0);
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
    EXPECT_EQ(portfolio.Shares("ACME"), 200);
    EXPECT_EQ(portfolio.Lots().front().underlying, "ACME");
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
    EXPECT_EQ(portfolio.Shares("ACME"), 0);
    EXPECT_TRUE(portfolio.Lots().empty());
}

TEST_F(TestPortfolio, assignment_moves_value_between_cash_and_shares_without_creating_any)
{
    const auto spot = Euro(36.50);

    portfolio.SellPut(Put(40.00), 2);

    const auto before = Equity(portfolio, "ACME", spot);

    portfolio.Assign(Put(40.00));

    EXPECT_EQ(Equity(portfolio, "ACME", spot), before);
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
    EXPECT_EQ(portfolio.Shares("ACME"), 100);
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

    EXPECT_EQ(Equity(portfolio, "ACME", Euro(40.00)), Euro(20000.00));
    EXPECT_EQ(Equity(portfolio, "ACME", Euro(45.00)), Euro(20500.00));
    EXPECT_EQ(Equity(portfolio, "ACME", Euro(35.00)), Euro(19500.00));
}

TEST_F(TestPortfolio, equity_carries_a_short_position_at_what_it_would_cost_to_settle)
{
    portfolio.SellPut(Put(40.00), 2);

    EXPECT_EQ(Equity(portfolio, "ACME", Euro(45.00)), Euro(20000.00));
    EXPECT_EQ(Equity(portfolio, "ACME", Euro(38.00)), Euro(19600.00));
}

TEST_F(TestPortfolio, shares_of_one_underlying_do_not_cover_a_call_on_another)
{
    portfolio.SellPut(Put("ACME", 40.00), 1);
    portfolio.Assign(Put("ACME", 40.00));

    EXPECT_EQ(portfolio.Shares("ACME"), 100);
    EXPECT_EQ(portfolio.Shares("BETA"), 0);
    EXPECT_EQ(portfolio.UncoveredShares("BETA"), 0);
    EXPECT_DEATH(portfolio.SellCall(Call("BETA", 45.00), 1), "");
}

TEST_F(TestPortfolio, a_call_covered_by_one_underlying_leaves_the_other_free_to_be_written_against)
{
    portfolio.SellPut(Put("ACME", 40.00), 1);
    portfolio.Assign(Put("ACME", 40.00));
    portfolio.SellPut(Put("BETA", 50.00), 1);
    portfolio.Assign(Put("BETA", 50.00));
    portfolio.SellCall(Call("ACME", 45.00), 1);

    EXPECT_EQ(portfolio.CoveredShares("ACME"), 100);
    EXPECT_EQ(portfolio.UncoveredShares("ACME"), 0);
    EXPECT_EQ(portfolio.CoveredShares("BETA"), 0);
    EXPECT_EQ(portfolio.UncoveredShares("BETA"), 100);

    portfolio.SellCall(Call("BETA", 55.00), 1);

    EXPECT_EQ(portfolio.CoveredShares("BETA"), 100);
}

TEST_F(TestPortfolio, shares_leave_the_underlying_they_belong_to_oldest_lot_first)
{
    portfolio.SellPut(Put("ACME", 40.00), 1);
    portfolio.Assign(Put("ACME", 40.00));
    portfolio.SellPut(Put("BETA", 50.00), 1);
    portfolio.Assign(Put("BETA", 50.00));
    portfolio.SellPut(Put("ACME", 60.00), 1);
    portfolio.Assign(Put("ACME", 60.00));

    portfolio.SellCall(Call("ACME", 70.00), 1);
    portfolio.Assign(Call("ACME", 70.00));

    ASSERT_EQ(portfolio.Lots().size(), 2u);
    EXPECT_EQ(portfolio.Lots().front().underlying, "BETA");
    EXPECT_EQ(portfolio.Lots().back().pricePerShare, Euro(60.00));
    EXPECT_EQ(portfolio.Shares("ACME"), 100);
    EXPECT_EQ(portfolio.Shares("BETA"), 100);
}

TEST_F(TestPortfolio, equity_values_every_underlying_at_its_own_price)
{
    portfolio.SellPut(Put("ACME", 40.00), 1);
    portfolio.Assign(Put("ACME", 40.00));
    portfolio.SellPut(Put("BETA", 50.00), 1);
    portfolio.Assign(Put("BETA", 50.00));

    const std::array prices{ SpotPrice{ "ACME", Euro(45.00) }, SpotPrice{ "BETA", Euro(48.00) } };

    EXPECT_EQ(Equity(portfolio, prices), Euro(20300.00));
}

TEST_F(TestPortfolio, assignment_creates_no_value_while_another_underlying_is_held)
{
    portfolio.SellPut(Put("BETA", 50.00), 1);
    portfolio.Assign(Put("BETA", 50.00));
    portfolio.SellPut(Put("ACME", 40.00), 2);

    const std::array prices{ SpotPrice{ "ACME", Euro(36.50) }, SpotPrice{ "BETA", Euro(52.25) } };
    const auto before = Equity(portfolio, prices);

    portfolio.Assign(Put("ACME", 40.00));

    EXPECT_EQ(Equity(portfolio, prices), before);
}

TEST_F(TestPortfolio, a_holding_that_was_given_no_price_cannot_be_valued)
{
    portfolio.SellPut(Put("ACME", 40.00), 1);
    portfolio.Assign(Put("ACME", 40.00));
    portfolio.SellPut(Put("BETA", 50.00), 1);

    const std::array quoted{ SpotPrice{ "ACME", Euro(45.00) } };

    EXPECT_DEATH(Equity(portfolio, quoted), "");
    EXPECT_DEATH(Equity(portfolio, "BETA", Euro(50.00)), "");
}

TEST_F(TestPortfolio, an_underlying_cannot_be_given_two_prices)
{
    const std::array twice{ SpotPrice{ "ACME", Euro(45.00) }, SpotPrice{ "ACME", Euro(46.00) } };

    EXPECT_DEATH(Equity(portfolio, twice), "");
}
