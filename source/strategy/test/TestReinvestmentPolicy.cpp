#include "source/strategy/ReinvestmentPolicy.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::strategy::ReinvestmentPolicy;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    class TestReinvestmentPolicy
        : public ::testing::Test
    {
    protected:
        static constexpr std::int32_t fivePercent = 500;

        ReinvestmentPolicy fullyInvested{ ReinvestmentPolicy::WithCashBuffer(0) };
        ReinvestmentPolicy buffered{ ReinvestmentPolicy::WithCashBuffer(fivePercent) };
    };
}

TEST_F(TestReinvestmentPolicy, a_buffer_that_is_not_a_fraction_is_refused)
{
    EXPECT_DEATH(ReinvestmentPolicy::WithCashBuffer(-1), "");
    EXPECT_DEATH(ReinvestmentPolicy::WithCashBuffer(10001), "");
}

TEST_F(TestReinvestmentPolicy, without_a_buffer_the_whole_balance_is_investable)
{
    EXPECT_EQ(fullyInvested.Retained(Euro(10000.00)), Euro(0.00));
    EXPECT_EQ(fullyInvested.Investable(Euro(10000.00)), Euro(10000.00));
}

TEST_F(TestReinvestmentPolicy, the_buffer_is_held_back_before_anything_is_committed)
{
    EXPECT_EQ(buffered.Retained(Euro(10000.00)), Euro(500.00));
    EXPECT_EQ(buffered.Investable(Euro(10000.00)), Euro(9500.00));
}

TEST_F(TestReinvestmentPolicy, what_is_held_back_and_what_is_committed_add_back_to_the_balance)
{
    for (const auto amount : { 0.00, 0.01, 7.77, 1234.56, 98765.43 })
    {
        const auto balance = Euro(amount);

        EXPECT_EQ(buffered.Retained(balance) + buffered.Investable(balance), balance);
    }
}

TEST_F(TestReinvestmentPolicy, a_balance_short_of_one_contract_opens_none)
{
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(3999.99), Euro(4000.00)), 0);
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(0.00), Euro(4000.00)), 0);
}

TEST_F(TestReinvestmentPolicy, a_balance_of_exactly_one_contract_opens_one)
{
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(4000.00), Euro(4000.00)), 1);
}

TEST_F(TestReinvestmentPolicy, the_number_of_contracts_grows_with_the_balance)
{
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(8000.00), Euro(4000.00)), 2);
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(11999.99), Euro(4000.00)), 2);
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(12000.00), Euro(4000.00)), 3);
}

TEST_F(TestReinvestmentPolicy, the_buffer_costs_the_contract_it_would_have_paid_for)
{
    EXPECT_EQ(buffered.ContractsFor(Euro(4000.00), Euro(4000.00)), 0);
    EXPECT_EQ(buffered.ContractsFor(Euro(4210.53), Euro(4000.00)), 1);
}

TEST_F(TestReinvestmentPolicy, holding_everything_back_commits_nothing)
{
    const auto everything = ReinvestmentPolicy::WithCashBuffer(10000);

    EXPECT_EQ(everything.Investable(Euro(10000.00)), Euro(0.00));
    EXPECT_EQ(everything.ContractsFor(Euro(10000.00), Euro(4000.00)), 0);
}

TEST_F(TestReinvestmentPolicy, a_contract_that_costs_nothing_to_secure_is_refused)
{
    EXPECT_DEATH(fullyInvested.ContractsFor(Euro(10000.00), Euro(0.00)), "");
    EXPECT_DEATH(fullyInvested.ContractsFor(Euro(-1.00), Euro(4000.00)), "");
}
