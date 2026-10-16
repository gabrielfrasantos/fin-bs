#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include "source/core/OptionContract.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::core::OptionContract;
    using fin::core::OptionType;

    class TestOptionContract
        : public ::testing::Test
    {
    protected:
        OptionContract Put(int64_t strikeCents, uint32_t multiplier = OptionContract::standardMultiplier) const
        {
            return OptionContract("AAPL", expiry, Money::FromCents(strikeCents, Currency::Eur), OptionType::Put, multiplier);
        }

        const Date expiry = Date::FromYearMonthDay(2025, 3, 21);
    };
}

TEST_F(TestOptionContract, keeps_the_identity_it_was_given)
{
    const auto contract = Put(18000);

    EXPECT_EQ(contract.Underlying(), "AAPL");
    EXPECT_EQ(contract.Expiry(), expiry);
    EXPECT_EQ(contract.Strike(), Money::FromCents(18000, Currency::Eur));
    EXPECT_EQ(contract.Type(), OptionType::Put);
    EXPECT_EQ(contract.Multiplier(), 100u);
}

TEST_F(TestOptionContract, applies_the_multiplier_to_the_notional)
{
    EXPECT_EQ(Put(18000).Notional(), Money::FromCents(1800000, Currency::Eur));
    EXPECT_EQ(Put(18000, 10).Notional(), Money::FromCents(180000, Currency::Eur));
    EXPECT_EQ(Put(1).Notional(), Money::FromCents(100, Currency::Eur));
}

TEST_F(TestOptionContract, the_notional_keeps_the_currency_of_the_strike)
{
    const auto contract = OptionContract("SPY", expiry, Money::FromCents(50000, Currency::Usd), OptionType::Call);

    EXPECT_EQ(contract.Notional().GetCurrency(), Currency::Usd);
    EXPECT_EQ(contract.Notional().Cents(), 5000000);
}

TEST_F(TestOptionContract, compares_equal_only_when_every_field_matches)
{
    EXPECT_TRUE(Put(18000) == Put(18000));
    EXPECT_TRUE(Put(18000) != Put(18500));
    EXPECT_TRUE(Put(18000) != Put(18000, 50));
    EXPECT_TRUE(Put(18000) != OptionContract("AAPL", expiry, Money::FromCents(18000, Currency::Eur), OptionType::Call));
    EXPECT_TRUE(Put(18000) != OptionContract("MSFT", expiry, Money::FromCents(18000, Currency::Eur), OptionType::Put));
}

TEST_F(TestOptionContract, orders_by_underlying_then_expiry_then_strike)
{
    const auto laterExpiry = expiry.AddDays(28);

    EXPECT_TRUE(Put(18000) < OptionContract("MSFT", expiry, Money::FromCents(1, Currency::Eur), OptionType::Put));
    EXPECT_TRUE(Put(18000) < OptionContract("AAPL", laterExpiry, Money::FromCents(1, Currency::Eur), OptionType::Put));
    EXPECT_TRUE(Put(18000) < Put(18500));
    EXPECT_TRUE(OptionContract("AAPL", expiry, Money::FromCents(18000, Currency::Eur), OptionType::Call) < Put(18000));
}

TEST_F(TestOptionContract, rejects_a_contract_that_cannot_exist)
{
    const auto strike = Money::FromCents(18000, Currency::Eur);

    EXPECT_DEATH(static_cast<void>(OptionContract("", expiry, strike, OptionType::Put)), "");
    EXPECT_DEATH(static_cast<void>(OptionContract("AAPL", expiry, Money::Zero(Currency::Eur), OptionType::Put)), "");
    EXPECT_DEATH(static_cast<void>(OptionContract("AAPL", expiry, strike, OptionType::Put, 0)), "");
}
