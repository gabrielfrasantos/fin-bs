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
        ReinvestmentPolicy capped{ ReinvestmentPolicy::WithCashBufferAndCap(0, 1500) };
    };
}

TEST_F(TestReinvestmentPolicy, a_cap_that_is_not_a_fraction_of_the_book_is_refused)
{
    EXPECT_DEATH(ReinvestmentPolicy::WithCashBufferAndCap(0, 0), "");
    EXPECT_DEATH(ReinvestmentPolicy::WithCashBufferAndCap(0, 10001), "");
}

TEST_F(TestReinvestmentPolicy, a_policy_stated_without_a_cap_lets_one_name_take_the_whole_book)
{
    EXPECT_EQ(fullyInvested.CapBasisPoints(), 10000);
    EXPECT_EQ(fullyInvested.Ceiling(Euro(100000.00)), Euro(100000.00));
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(100000.00), Euro(4688.00), Euro(100000.00)), 21);
}

TEST_F(TestReinvestmentPolicy, the_cap_limits_a_name_to_its_share_of_the_opening_equity)
{
    EXPECT_EQ(capped.Ceiling(Euro(100000.00)), Euro(15000.00));
    EXPECT_EQ(capped.ContractsFor(Euro(100000.00), Euro(4688.00), Euro(100000.00)), 3);
    EXPECT_EQ(capped.ContractsFor(Euro(100000.00), Euro(15000.00), Euro(100000.00)), 1);
    EXPECT_EQ(capped.ContractsFor(Euro(100000.00), Euro(15000.01), Euro(100000.00)), 0);
}

TEST_F(TestReinvestmentPolicy, the_cap_is_measured_against_the_whole_book_and_the_cash_still_binds)
{
    EXPECT_EQ(capped.ContractsFor(Euro(20000.00), Euro(4000.00), Euro(200000.00)), 5);
    EXPECT_EQ(capped.ContractsFor(Euro(20000.00), Euro(4000.00), Euro(20000.00)), 0);
    EXPECT_EQ(capped.ContractsFor(Euro(20000.00), Euro(4000.00), Euro(-100.00)), 0);
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
}

TEST_F(TestReinvestmentPolicy, a_balance_below_nothing_opens_no_contract)
{
    EXPECT_EQ(fullyInvested.ContractsFor(Euro(-0.01), Euro(4000.00)), 0);
    EXPECT_EQ(buffered.ContractsFor(Euro(-5000.00), Euro(4000.00)), 0);
}
