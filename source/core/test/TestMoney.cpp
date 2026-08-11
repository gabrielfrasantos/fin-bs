#include "source/core/Money.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;

    Money Eur(int64_t cents)
    {
        return Money::FromCents(cents, Currency::Eur);
    }

    Money Usd(int64_t cents)
    {
        return Money::FromCents(cents, Currency::Usd);
    }

    class TestMoney
        : public ::testing::Test
    {
    protected:
        static constexpr int64_t int64Max = std::numeric_limits<int64_t>::max();
        static constexpr int64_t int64Min = std::numeric_limits<int64_t>::min();
    };
}

TEST_F(TestMoney, is_constructed_from_cents_and_keeps_them_exactly)
{
    EXPECT_EQ(Eur(1234).Cents(), 1234);
    EXPECT_EQ(Eur(-1234).Cents(), -1234);
    EXPECT_EQ(Money::Zero(Currency::Usd).Cents(), 0);
    EXPECT_EQ(Money::Zero(Currency::Usd).GetCurrency(), Currency::Usd);
}

TEST_F(TestMoney, is_usable_in_a_constant_expression)
{
    static_assert(Money::FromCents(150, Currency::Eur).Cents() == 150);
    static_assert((Money::FromCents(150, Currency::Eur) + Money::FromCents(50, Currency::Eur)).Cents() == 200);
    static_assert((Money::FromCents(5, Currency::Eur) / 2).Cents() == 2);
}

TEST_F(TestMoney, converts_an_amount_to_cents_with_half_to_even_rounding)
{
    EXPECT_EQ(Money::FromAmount(12.345, Currency::Eur).Cents(), 1234);
    EXPECT_EQ(Money::FromAmount(0.005, Currency::Eur).Cents(), 0);
    EXPECT_EQ(Money::FromAmount(0.015, Currency::Eur).Cents(), 2);
    EXPECT_EQ(Money::FromAmount(-0.005, Currency::Eur).Cents(), 0);
    EXPECT_EQ(Money::FromAmount(-0.015, Currency::Eur).Cents(), -2);
    EXPECT_EQ(Money::FromAmount(1.234, Currency::Eur).Cents(), 123);
    EXPECT_EQ(Money::FromAmount(1.236, Currency::Eur).Cents(), 124);
}

TEST_F(TestMoney, adds_and_subtracts_within_a_currency)
{
    EXPECT_EQ((Eur(1050) + Eur(275)).Cents(), 1325);
    EXPECT_EQ((Eur(1050) - Eur(275)).Cents(), 775);
    EXPECT_EQ((-Eur(1050)).Cents(), -1050);

    auto running = Eur(0);
    running += Eur(345);
    running -= Eur(45);
    EXPECT_EQ(running.Cents(), 300);
}

TEST_F(TestMoney, scales_by_an_integer_quantity)
{
    EXPECT_EQ((Eur(345) * 3).Cents(), 1035);
    EXPECT_EQ((Eur(18000) * 0).Cents(), 0);
    EXPECT_EQ((Eur(345) * -2).Cents(), -690);
}

TEST_F(TestMoney, scales_by_a_factor_with_half_to_even_rounding)
{
    EXPECT_EQ((Eur(1234) * 0.015).Cents(), 19);
    EXPECT_EQ((Eur(5) * 0.5).Cents(), 2);
    EXPECT_EQ((Eur(7) * 0.5).Cents(), 4);
    EXPECT_EQ((Eur(-5) * 0.5).Cents(), -2);
    EXPECT_EQ((Eur(-7) * 0.5).Cents(), -4);
}

TEST_F(TestMoney, divides_with_half_to_even_rounding)
{
    EXPECT_EQ((Eur(5) / 3).Cents(), 2);
    EXPECT_EQ((Eur(5) / 2).Cents(), 2);
    EXPECT_EQ((Eur(7) / 2).Cents(), 4);
    EXPECT_EQ((Eur(-5) / 2).Cents(), -2);
    EXPECT_EQ((Eur(-7) / 2).Cents(), -4);
    EXPECT_EQ((Eur(-5) / -2).Cents(), 2);
    EXPECT_EQ((Eur(1000) / 4).Cents(), 250);
}

TEST_F(TestMoney, accumulates_ten_thousand_amounts_exactly_and_order_independently)
{
    std::mt19937 generator(42);
    std::vector<int64_t> amounts;
    amounts.reserve(10000);

    for (auto i = 0; i != 10000; ++i)
        amounts.push_back(static_cast<int64_t>(generator() % 200000u) - 100000);

    const auto reference = std::accumulate(amounts.begin(), amounts.end(), int64_t{ 0 });

    auto forwardSum = Money::Zero(Currency::Eur);
    for (auto amount : amounts)
        forwardSum += Eur(amount);

    auto reverseSum = Money::Zero(Currency::Eur);
    for (auto amount = amounts.rbegin(); amount != amounts.rend(); ++amount)
        reverseSum += Eur(*amount);

    EXPECT_EQ(forwardSum.Cents(), reference);
    EXPECT_EQ(reverseSum.Cents(), reference);
}

TEST_F(TestMoney, converts_to_double_at_the_boundary)
{
    EXPECT_DOUBLE_EQ(Eur(1234).ToDouble(), 12.34);
    EXPECT_DOUBLE_EQ(Eur(-50).ToDouble(), -0.5);
    EXPECT_DOUBLE_EQ(Money::Zero(Currency::Eur).ToDouble(), 0.0);
}

TEST_F(TestMoney, compares_within_a_currency)
{
    EXPECT_TRUE(Eur(100) == Eur(100));
    EXPECT_TRUE(Eur(100) != Eur(101));
    EXPECT_TRUE(Eur(100) < Eur(101));
    EXPECT_TRUE(Eur(-1) < Eur(0));
    EXPECT_TRUE(Eur(101) >= Eur(101));
}

TEST_F(TestMoney, rejects_mixing_currencies)
{
    EXPECT_DEATH(static_cast<void>(Eur(100) + Usd(100)), "");
    EXPECT_DEATH(static_cast<void>(Eur(100) - Usd(100)), "");
    EXPECT_DEATH(static_cast<void>(Eur(100) < Usd(100)), "");
    EXPECT_DEATH(static_cast<void>(Eur(100) == Usd(100)), "");
}

TEST_F(TestMoney, rejects_overflowing_arithmetic)
{
    EXPECT_DEATH(static_cast<void>(Eur(int64Max) + Eur(1)), "");
    EXPECT_DEATH(static_cast<void>(Eur(int64Min) - Eur(1)), "");
    EXPECT_DEATH(static_cast<void>(Eur(int64Max) * 2), "");
    EXPECT_DEATH(static_cast<void>(-Eur(int64Min)), "");
    EXPECT_DEATH(static_cast<void>(Eur(100) / 0), "");
}
