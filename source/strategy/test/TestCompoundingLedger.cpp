#include "source/strategy/CompoundingLedger.hpp"
#include "source/strategy/Portfolio.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::core::OptionContract;
    using fin::core::OptionType;
    using fin::strategy::CashflowKind;
    using fin::strategy::CompoundingLedger;
    using fin::strategy::Equity;
    using fin::strategy::Portfolio;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date Day(uint32_t day)
    {
        return Date::FromYearMonthDay(2025, 6, day);
    }

    class TestCompoundingLedger
        : public ::testing::Test
    {
    protected:
        CompoundingLedger ledger{ Currency::Eur };
    };
}

TEST_F(TestCompoundingLedger, a_ledger_with_no_entries_nets_to_nothing)
{
    EXPECT_TRUE(ledger.Entries().empty());
    EXPECT_EQ(ledger.Net(), Euro(0.00));
    EXPECT_EQ(ledger.NetOf(CashflowKind::Premium), Euro(0.00));
}

TEST_F(TestCompoundingLedger, entries_are_kept_in_the_order_they_were_posted)
{
    ledger.Post(Day(2), CashflowKind::Premium, Euro(160.00));
    ledger.Post(Day(2), CashflowKind::Fee, Euro(-1.50));
    ledger.Post(Day(20), CashflowKind::Dividend, Euro(12.00));

    ASSERT_EQ(ledger.Entries().size(), 3u);
    EXPECT_EQ(ledger.Entries()[0].amount, Euro(160.00));
    EXPECT_EQ(ledger.Entries()[1].kind, CashflowKind::Fee);
    EXPECT_EQ(ledger.Entries()[2].on, Day(20));
}

TEST_F(TestCompoundingLedger, the_net_of_a_kind_counts_only_that_kind)
{
    ledger.Post(Day(2), CashflowKind::Premium, Euro(160.00));
    ledger.Post(Day(3), CashflowKind::Fee, Euro(-1.50));
    ledger.Post(Day(4), CashflowKind::Premium, Euro(-40.00));
    ledger.Post(Day(5), CashflowKind::Tax, Euro(-90.00));

    EXPECT_EQ(ledger.NetOf(CashflowKind::Premium), Euro(120.00));
    EXPECT_EQ(ledger.NetOf(CashflowKind::Fee), Euro(-1.50));
    EXPECT_EQ(ledger.NetOf(CashflowKind::Assignment), Euro(0.00));
    EXPECT_EQ(ledger.Net(), Euro(28.50));
}

TEST_F(TestCompoundingLedger, a_premium_and_an_assignment_may_go_either_way)
{
    ledger.Post(Day(2), CashflowKind::Premium, Euro(160.00));
    ledger.Post(Day(3), CashflowKind::Premium, Euro(-40.00));
    ledger.Post(Day(4), CashflowKind::Assignment, Euro(-8800.00));
    ledger.Post(Day(5), CashflowKind::Assignment, Euro(9200.00));

    EXPECT_EQ(ledger.Entries().size(), 4u);
}

TEST_F(TestCompoundingLedger, a_charge_can_never_be_a_receipt)
{
    EXPECT_DEATH(ledger.Post(Day(2), CashflowKind::Fee, Euro(1.50)), "");
    EXPECT_DEATH(ledger.Post(Day(2), CashflowKind::Tax, Euro(90.00)), "");
    EXPECT_DEATH(ledger.Post(Day(2), CashflowKind::Fx, Euro(0.25)), "");
}

TEST_F(TestCompoundingLedger, a_dividend_can_never_be_a_charge)
{
    EXPECT_DEATH(ledger.Post(Day(2), CashflowKind::Dividend, Euro(-12.00)), "");
}

TEST_F(TestCompoundingLedger, an_entry_cannot_be_dated_before_the_one_before_it)
{
    ledger.Post(Day(10), CashflowKind::Premium, Euro(160.00));

    EXPECT_DEATH(ledger.Post(Day(9), CashflowKind::Premium, Euro(10.00)), "");
}

TEST_F(TestCompoundingLedger, an_entry_in_another_currency_is_refused)
{
    EXPECT_DEATH(ledger.Post(Day(2), CashflowKind::Premium, Money::FromAmount(160.00, Currency::Usd)), "");
}

TEST_F(TestCompoundingLedger, the_ledger_accounts_for_every_movement_of_cash)
{
    const auto opening = Euro(10000.00);
    const OptionContract put("ACME", Day(20), Euro(44.00), OptionType::Put);

    auto portfolio = Portfolio::Funded(opening);

    portfolio.SellPut(put, 2);
    portfolio.Receive(Euro(180.00));
    ledger.Post(Day(2), CashflowKind::Premium, Euro(180.00));
    portfolio.Pay(Euro(1.50));
    ledger.Post(Day(2), CashflowKind::Fee, Euro(-1.50));

    portfolio.Assign(put);
    ledger.Post(Day(20), CashflowKind::Assignment, Euro(-8800.00));
    portfolio.Pay(Euro(3.00));
    ledger.Post(Day(20), CashflowKind::Fee, Euro(-3.00));

    EXPECT_EQ(portfolio.Cash(), opening + ledger.Net());
    EXPECT_EQ(Equity(portfolio, Euro(40.00)), opening + ledger.Net() + Euro(40.00) * portfolio.Shares());
}
