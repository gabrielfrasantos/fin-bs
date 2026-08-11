#include "source/strategy/Account.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::strategy::Account;
    using fin::strategy::CashflowKind;
    using fin::strategy::Portfolio;
    using fin::strategy::Reconciles;
    using fin::tax::Box3Parameters;
    using fin::tax::Box3TaxModel;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
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

    class TestAccount
        : public ::testing::Test
    {
    protected:
        Account account{ Portfolio::Funded(Euro(10000.00)), Rules() };
    };
}

TEST_F(TestAccount, an_account_opens_at_the_balance_it_was_funded_with)
{
    EXPECT_EQ(account.Opening(), Euro(10000.00));
    EXPECT_EQ(account.Holdings().Cash(), Euro(10000.00));
    EXPECT_EQ(account.GetCurrency(), Currency::Eur);
    EXPECT_TRUE(account.Ledger().Entries().empty());
    EXPECT_TRUE(Reconciles(account));
}

TEST_F(TestAccount, an_account_reconciles_while_every_movement_of_cash_is_also_posted)
{
    account.Holdings().Receive(Euro(160.00));
    account.Ledger().Post(On(2025, 1, 3), CashflowKind::Premium, Euro(160.00));

    EXPECT_TRUE(Reconciles(account));

    account.Holdings().Pay(Euro(1.50));
    account.Ledger().Post(On(2025, 1, 3), CashflowKind::Fee, Euro(-1.50));

    EXPECT_TRUE(Reconciles(account));
    EXPECT_EQ(account.Holdings().Cash(), Euro(10158.50));
}

TEST_F(TestAccount, an_account_stops_reconciling_when_cash_moves_unrecorded)
{
    account.Holdings().Pay(Euro(1.50));

    EXPECT_FALSE(Reconciles(account));
}

TEST_F(TestAccount, an_account_carries_the_tax_rules_it_was_opened_under)
{
    EXPECT_EQ(account.Taxes().Year(), 2025);
}

TEST_F(TestAccount, a_copy_of_an_account_is_a_separate_account)
{
    auto copy = account;

    copy.Holdings().Receive(Euro(500.00));
    copy.Ledger().Post(On(2025, 1, 3), CashflowKind::Premium, Euro(500.00));

    EXPECT_EQ(account.Holdings().Cash(), Euro(10000.00));
    EXPECT_TRUE(account.Ledger().Entries().empty());
    EXPECT_TRUE(Reconciles(copy));
}
