#include "source/tax/DividendTax.hpp"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::tax::Box3Parameters;
    using fin::tax::Dividend;
    using fin::tax::DividendTax;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Box3Parameters PublishedForTwentyTwentyFive()
    {
        return { .taxYear = 2025,
            .taxFreeAllowance = Euro(57684.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 };
    }

    Dividend PaidAtHome(double gross)
    {
        return { Euro(gross), 1500 };
    }

    Dividend PaidAbroad(double gross, std::int32_t withheldBasisPoints)
    {
        return { Euro(gross), withheldBasisPoints };
    }

    class TestDividendTax
        : public ::testing::Test
    {
    protected:
        DividendTax tax{ PublishedForTwentyTwentyFive() };
    };
}

TEST_F(TestDividendTax, a_domestic_dividend_is_withheld_at_the_published_rate)
{
    EXPECT_EQ(tax.Withheld(PaidAtHome(1000.00)), Euro(150.00));
    EXPECT_EQ(tax.Received(PaidAtHome(1000.00)), Euro(850.00));
}

TEST_F(TestDividendTax, what_is_withheld_and_what_is_received_add_up_to_what_was_declared)
{
    const auto dividend = PaidAtHome(1234.57);

    EXPECT_EQ(tax.Withheld(dividend) + tax.Received(dividend), dividend.gross);
}

TEST_F(TestDividendTax, a_domestic_dividend_is_credited_in_full_and_leaks_nothing)
{
    EXPECT_EQ(tax.Creditable(PaidAtHome(1000.00)), Euro(150.00));
    EXPECT_EQ(tax.Leakage(PaidAtHome(1000.00)), Euro(0.00));
}

TEST_F(TestDividendTax, a_foreign_dividend_withheld_above_the_creditable_rate_leaks_the_excess)
{
    const auto german = PaidAbroad(1000.00, 2638);

    EXPECT_EQ(tax.Withheld(german), Euro(263.80));
    EXPECT_EQ(tax.Creditable(german), Euro(150.00));
    EXPECT_EQ(tax.Leakage(german), Euro(113.80));
}

TEST_F(TestDividendTax, a_foreign_dividend_withheld_below_the_creditable_rate_credits_only_what_was_withheld)
{
    const auto british = PaidAbroad(1000.00, 0);

    EXPECT_EQ(tax.Withheld(british), Euro(0.00));
    EXPECT_EQ(tax.Creditable(british), Euro(0.00));
    EXPECT_EQ(tax.Leakage(british), Euro(0.00));
}

TEST_F(TestDividendTax, what_is_withheld_is_what_is_credited_plus_what_leaks)
{
    const auto dividend = PaidAbroad(777.77, 3000);

    EXPECT_EQ(tax.Creditable(dividend) + tax.Leakage(dividend), tax.Withheld(dividend));
}

TEST_F(TestDividendTax, a_credit_is_taken_against_the_wealth_tax_of_the_same_year)
{
    const std::array<Dividend, 2> received{ { PaidAtHome(1000.00), PaidAtHome(400.00) } };

    EXPECT_EQ(tax.CreditAgainst(Euro(758.34), received), Euro(210.00));
}

TEST_F(TestDividendTax, a_credit_larger_than_the_wealth_tax_is_capped_at_it_and_is_not_repaid)
{
    const std::array<Dividend, 1> received{ { PaidAtHome(10000.00) } };

    EXPECT_EQ(tax.CreditAgainst(Euro(758.34), received), Euro(758.34));
}

TEST_F(TestDividendTax, a_year_without_dividends_takes_no_credit)
{
    EXPECT_EQ(tax.CreditAgainst(Euro(758.34), {}), Euro(0.00));
}

TEST_F(TestDividendTax, what_leaks_abroad_is_a_cost_the_wealth_tax_never_returns)
{
    const std::array<Dividend, 1> abroad{ { PaidAbroad(1000.00, 2638) } };
    const std::array<Dividend, 1> home{ { PaidAtHome(1000.00) } };

    EXPECT_EQ(tax.CreditAgainst(Euro(758.34), abroad), tax.CreditAgainst(Euro(758.34), home));
    EXPECT_GT(tax.Leakage(abroad.front()), tax.Leakage(home.front()));
}

TEST_F(TestDividendTax, a_dividend_that_was_never_paid_is_refused)
{
    EXPECT_DEATH(tax.Withheld(PaidAtHome(-1.00)), "");
}

TEST_F(TestDividendTax, a_dividend_in_another_currency_than_the_rules_were_written_in_is_refused)
{
    const Dividend foreign{ Money::FromAmount(1000.00, Currency::Usd), 1500 };

    EXPECT_DEATH(tax.Withheld(foreign), "");
}
