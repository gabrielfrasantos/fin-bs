#include "source/estimation/LogReturns.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::estimation::LogPrices;
    using fin::estimation::LogReturns;

    std::vector<Money> Prices(std::initializer_list<double> amounts)
    {
        std::vector<Money> result;

        for (auto amount : amounts)
            result.push_back(Money::FromAmount(amount, Currency::Eur));

        return result;
    }

    class TestLogReturns
        : public ::testing::Test
    {};
}

TEST_F(TestLogReturns, a_series_of_n_prices_yields_n_minus_one_returns)
{
    EXPECT_EQ(LogReturns(Prices({ 100.0, 101.0, 99.0 })).size(), 2u);
    EXPECT_EQ(LogPrices(Prices({ 100.0, 101.0, 99.0 })).size(), 3u);
}

TEST_F(TestLogReturns, an_unchanged_price_has_a_zero_return)
{
    const auto returns = LogReturns(Prices({ 50.0, 50.0, 50.0 }));

    EXPECT_EQ(returns[0], 0.0);
    EXPECT_EQ(returns[1], 0.0);
}

TEST_F(TestLogReturns, returns_are_the_difference_of_the_log_prices)
{
    const auto prices = Prices({ 100.0, 112.5, 93.75, 120.0 });
    const auto logPrices = LogPrices(prices);
    const auto returns = LogReturns(prices);

    for (std::size_t i = 0; i != returns.size(); ++i)
        EXPECT_NEAR(returns[i], logPrices[i + 1] - logPrices[i], 1e-15);
}

TEST_F(TestLogReturns, returns_are_additive_over_consecutive_periods)
{
    const auto returns = LogReturns(Prices({ 100.0, 112.5, 93.75, 120.0 }));
    const auto total = LogReturns(Prices({ 100.0, 120.0 }));

    EXPECT_NEAR(returns[0] + returns[1] + returns[2], total[0], 1e-15);
}

TEST_F(TestLogReturns, a_doubling_and_a_halving_are_opposite_returns)
{
    const auto returns = LogReturns(Prices({ 40.0, 80.0, 40.0 }));

    EXPECT_DOUBLE_EQ(returns[0], std::log(2.0));
    EXPECT_DOUBLE_EQ(returns[1], -std::log(2.0));
}

TEST_F(TestLogReturns, log_prices_are_the_logarithm_of_the_amount)
{
    const auto logPrices = LogPrices(Prices({ 1.0, 100.0 }));

    EXPECT_EQ(logPrices[0], 0.0);
    EXPECT_DOUBLE_EQ(logPrices[1], std::log(100.0));
}

TEST_F(TestLogReturns, an_empty_series_has_no_returns_and_stops_the_run)
{
    EXPECT_DEATH(LogReturns({}), "");
    EXPECT_DEATH(LogPrices({}), "");
}

TEST_F(TestLogReturns, a_price_that_is_not_positive_stops_the_run)
{
    EXPECT_DEATH(LogReturns(Prices({ 100.0, 0.0 })), "");
    EXPECT_DEATH(LogReturns(Prices({ 100.0, -1.0 })), "");
}

TEST_F(TestLogReturns, mixing_currencies_stops_the_run)
{
    const std::vector<Money> mixed{ Money::FromAmount(100.0, Currency::Eur), Money::FromAmount(100.0, Currency::Usd) };

    EXPECT_DEATH(LogReturns(mixed), "");
}
