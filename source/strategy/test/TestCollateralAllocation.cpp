#include "source/strategy/CollateralAllocation.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::strategy::CollateralClaim;
    using fin::strategy::PremiumPerUnitOfCollateral;
    using fin::strategy::ServiceOrder;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    CollateralClaim FromCash(double premiumPerShare, double strike)
    {
        return { .fromCash = true, .premiumPerShare = premiumPerShare, .strike = Euro(strike) };
    }

    CollateralClaim FromShares(double premiumPerShare, double strike)
    {
        return { .fromCash = false, .premiumPerShare = premiumPerShare, .strike = Euro(strike) };
    }
}

TEST(TestCollateralAllocation, a_premium_is_measured_against_the_collateral_it_commits)
{
    EXPECT_DOUBLE_EQ(PremiumPerUnitOfCollateral(FromCash(0.80, 40.00)), 0.02);
    EXPECT_DOUBLE_EQ(PremiumPerUnitOfCollateral(FromCash(1.00, 50.00)), 0.02);
    EXPECT_DOUBLE_EQ(PremiumPerUnitOfCollateral(FromCash(0.00, 40.00)), 0.00);
}

TEST(TestCollateralAllocation, nothing_to_serve_is_an_empty_order)
{
    EXPECT_TRUE(ServiceOrder({}).empty());
}

TEST(TestCollateralAllocation, what_needs_no_cash_is_served_before_what_does)
{
    const std::vector claims{ FromCash(2.00, 40.00), FromShares(0.10, 45.00), FromCash(1.00, 40.00), FromShares(0.20, 30.00) };

    EXPECT_EQ(ServiceOrder(claims), (std::vector<std::size_t>{ 1, 3, 0, 2 }));
}

TEST(TestCollateralAllocation, the_richest_premium_for_the_collateral_it_commits_is_served_first)
{
    const std::vector claims{ FromCash(0.80, 40.00), FromCash(1.50, 50.00), FromCash(0.30, 20.00) };

    EXPECT_EQ(ServiceOrder(claims), (std::vector<std::size_t>{ 1, 0, 2 }));
}

TEST(TestCollateralAllocation, a_richer_premium_is_served_first_whatever_order_it_was_listed_in)
{
    const std::vector listedFirst{ FromCash(1.50, 50.00), FromCash(0.80, 40.00) };
    const std::vector listedLast{ FromCash(0.80, 40.00), FromCash(1.50, 50.00) };

    EXPECT_EQ(ServiceOrder(listedFirst), (std::vector<std::size_t>{ 0, 1 }));
    EXPECT_EQ(ServiceOrder(listedLast), (std::vector<std::size_t>{ 1, 0 }));
}

TEST(TestCollateralAllocation, an_exact_tie_is_served_in_the_order_it_was_listed)
{
    const std::vector claims{ FromCash(0.80, 40.00), FromCash(0.90, 40.00), FromCash(0.80, 40.00) };

    EXPECT_EQ(ServiceOrder(claims), (std::vector<std::size_t>{ 1, 0, 2 }));
}

TEST(TestCollateralAllocation, every_claim_is_served_exactly_once)
{
    const std::vector claims{ FromCash(0.80, 40.00), FromShares(0.10, 45.00), FromCash(1.50, 50.00), FromShares(0.20, 30.00), FromCash(0.30, 20.00) };

    auto order = ServiceOrder(claims);

    ASSERT_EQ(order.size(), claims.size());

    std::sort(order.begin(), order.end());

    EXPECT_EQ(order, (std::vector<std::size_t>{ 0, 1, 2, 3, 4 }));
}

TEST(TestCollateralAllocation, a_claim_against_a_worthless_strike_is_refused)
{
    EXPECT_DEATH(PremiumPerUnitOfCollateral(FromCash(0.80, 0.00)), "");
}
