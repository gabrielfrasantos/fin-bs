#include "source/pricing/BlackScholes.hpp"
#include "source/pricing/NormalDistribution.hpp"
#include "source/pricing/test/OptionScenario.hpp"
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <limits>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::core::OptionType;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::pricing::BlackScholes;
    using fin::pricing::OptionParameters;
    using fin::pricing::StandardNormalCdf;
    using fin::pricing::test::OptionScenario;

    class TestBlackScholes
        : public ::testing::Test
    {
    protected:
        static double Price(const OptionParameters& parameters, double annualizedVolatility)
        {
            return BlackScholes(parameters, Volatility::FromAnnualized(annualizedVolatility)).Price();
        }

        static double CarriedSpot(const OptionParameters& parameters)
        {
            return parameters.spot.ToDouble() * parameters.dividendYield.DiscountFactor(parameters.timeToExpiry);
        }

        static double DiscountedStrike(const OptionParameters& parameters)
        {
            return parameters.strike.ToDouble() * parameters.riskFreeRate.DiscountFactor(parameters.timeToExpiry);
        }
    };
}

TEST_F(TestBlackScholes, matches_published_reference_prices_without_a_dividend_yield)
{
    EXPECT_NEAR(Price(OptionScenario(42.0, 40.0, 0.10, 0.0, 0.5, OptionType::Call), 0.20), 4.76, 5e-3);
    EXPECT_NEAR(Price(OptionScenario(42.0, 40.0, 0.10, 0.0, 0.5, OptionType::Put), 0.20), 0.81, 5e-3);
}

TEST_F(TestBlackScholes, matches_published_reference_prices_with_a_dividend_yield)
{
    EXPECT_NEAR(Price(OptionScenario(100.0, 95.0, 0.10, 0.05, 0.5, OptionType::Put), 0.20), 2.4648, 5e-5);
    EXPECT_NEAR(Price(OptionScenario(75.0, 70.0, 0.10, 0.05, 0.5, OptionType::Put), 0.35), 4.0870, 5e-5);
}

TEST_F(TestBlackScholes, an_at_the_money_option_without_carry_matches_the_closed_form)
{
    const double expected = 100.0 * (2.0 * StandardNormalCdf(0.1) - 1.0);

    EXPECT_NEAR(Price(OptionScenario(100.0, 100.0, 0.0, 0.0, 1.0, OptionType::Call), 0.20), expected, 1e-12);
    EXPECT_NEAR(Price(OptionScenario(100.0, 100.0, 0.0, 0.0, 1.0, OptionType::Put), 0.20), expected, 1e-12);
}

TEST_F(TestBlackScholes, satisfies_put_call_parity)
{
    for (auto spot : { 60.0, 95.0, 100.0, 140.0 })
        for (auto volatility : { 0.05, 0.2, 0.9 })
            for (auto timeToExpiry : { 0.02, 0.5, 3.0 })
            {
                const auto call = OptionScenario(spot, 100.0, 0.03, 0.02, timeToExpiry, OptionType::Call);
                const auto put = OptionScenario(spot, 100.0, 0.03, 0.02, timeToExpiry, OptionType::Put);

                EXPECT_NEAR(Price(call, volatility) - Price(put, volatility), CarriedSpot(call) - DiscountedStrike(call), 1e-12);
            }
}

TEST_F(TestBlackScholes, prices_rise_with_volatility)
{
    for (auto type : { OptionType::Call, OptionType::Put })
    {
        const auto parameters = OptionScenario(105.0, 100.0, 0.03, 0.02, 0.75, type);
        double previous = -1.0;

        for (double volatility = 0.0; volatility < 1.5; volatility += 0.05)
        {
            const double price = Price(parameters, volatility);

            EXPECT_GT(price, previous);
            previous = price;
        }
    }
}

TEST_F(TestBlackScholes, a_call_rises_with_the_spot_while_a_put_falls)
{
    double previousCall = -1.0;
    double previousPut = std::numeric_limits<double>::max();

    for (double spot = 50.0; spot < 150.0; spot += 5.0)
    {
        const double call = Price(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, OptionType::Call), 0.25);
        const double put = Price(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, OptionType::Put), 0.25);

        EXPECT_GT(call, previousCall);
        EXPECT_LT(put, previousPut);

        previousCall = call;
        previousPut = put;
    }
}

TEST_F(TestBlackScholes, stays_inside_the_no_arbitrage_envelope)
{
    for (auto spot : { 40.0, 100.0, 250.0 })
        for (auto volatility : { 0.01, 0.3, 2.0 })
        {
            const auto call = OptionScenario(spot, 100.0, 0.04, 0.03, 1.25, OptionType::Call);
            const auto put = OptionScenario(spot, 100.0, 0.04, 0.03, 1.25, OptionType::Put);

            EXPECT_GE(Price(call, volatility), std::max(CarriedSpot(call) - DiscountedStrike(call), 0.0));
            EXPECT_LE(Price(call, volatility), CarriedSpot(call));
            EXPECT_GE(Price(put, volatility), std::max(DiscountedStrike(put) - CarriedSpot(put), 0.0));
            EXPECT_LE(Price(put, volatility), DiscountedStrike(put));
        }
}

TEST_F(TestBlackScholes, without_volatility_the_price_is_the_discounted_intrinsic_value_of_the_forward)
{
    for (auto spot : { 80.0, 100.0, 120.0 })
    {
        const auto call = OptionScenario(spot, 100.0, 0.05, 0.01, 2.0, OptionType::Call);
        const auto put = OptionScenario(spot, 100.0, 0.05, 0.01, 2.0, OptionType::Put);

        EXPECT_NEAR(Price(call, 0.0), std::max(CarriedSpot(call) - DiscountedStrike(call), 0.0), 1e-12);
        EXPECT_NEAR(Price(put, 0.0), std::max(DiscountedStrike(put) - CarriedSpot(put), 0.0), 1e-12);
    }
}

TEST_F(TestBlackScholes, at_expiry_the_price_is_the_intrinsic_value)
{
    for (auto spot : { 80.0, 100.0, 120.0 })
    {
        const auto call = OptionScenario(spot, 100.0, 0.05, 0.01, 0.0, OptionType::Call);
        const auto put = OptionScenario(spot, 100.0, 0.05, 0.01, 0.0, OptionType::Put);

        EXPECT_EQ(Price(call, 0.3), std::max(spot - 100.0, 0.0));
        EXPECT_EQ(Price(put, 0.3), std::max(100.0 - spot, 0.0));
    }
}

TEST_F(TestBlackScholes, the_two_standardised_moneyness_measures_differ_by_the_deviation_over_the_horizon)
{
    const auto parameters = OptionScenario(105.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);
    const BlackScholes model(parameters, Volatility::FromAnnualized(0.25));

    EXPECT_NEAR(model.Deviation(), 0.25 * std::sqrt(0.75), 1e-15);
    EXPECT_NEAR(model.D1() - model.D2(), model.Deviation(), 1e-15);
}

TEST_F(TestBlackScholes, splits_the_price_into_a_non_negative_intrinsic_and_extrinsic_part)
{
    for (auto spot : { 60.0, 100.0, 160.0 })
        for (auto type : { OptionType::Call, OptionType::Put })
        {
            const BlackScholes model(OptionScenario(spot, 100.0, 0.0, 0.0, 1.0, type), Volatility::FromAnnualized(0.25));

            EXPECT_GE(model.IntrinsicValue(), 0.0);
            EXPECT_GT(model.ExtrinsicValue(), 0.0);
            EXPECT_NEAR(model.IntrinsicValue() + model.ExtrinsicValue(), model.Price(), 1e-12);
        }
}

TEST_F(TestBlackScholes, refuses_inputs_that_cannot_describe_an_option)
{
    const auto parameters = OptionScenario(105.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);
    const auto volatility = Volatility::FromAnnualized(0.25);

    auto negativeTime = parameters;
    negativeTime.timeToExpiry = -0.01;
    EXPECT_DEATH(static_cast<void>(BlackScholes(negativeTime, volatility).Price()), "");

    auto negativeDividendYield = parameters;
    negativeDividendYield.dividendYield = Rate::FromContinuous(-0.01);
    EXPECT_DEATH(static_cast<void>(BlackScholes(negativeDividendYield, volatility).Price()), "");

    auto worthlessSpot = parameters;
    worthlessSpot.spot = Money::Zero(Currency::Eur);
    EXPECT_DEATH(static_cast<void>(BlackScholes(worthlessSpot, volatility).Price()), "");

    auto foreignStrike = parameters;
    foreignStrike.strike = Money::FromAmount(100.0, Currency::Usd);
    EXPECT_DEATH(static_cast<void>(BlackScholes(foreignStrike, volatility).Price()), "");
}
