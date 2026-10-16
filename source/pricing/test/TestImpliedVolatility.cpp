#include "source/pricing/BlackScholes.hpp"
#include "source/pricing/ImpliedVolatility.hpp"
#include "source/pricing/test/OptionScenario.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <limits>

namespace
{
    using fin::core::OptionType;
    using fin::core::Volatility;
    using fin::pricing::BlackScholes;
    using fin::pricing::ImpliedVolatility;
    using fin::pricing::OptionParameters;
    using fin::pricing::test::OptionScenario;

    class TestImpliedVolatility
        : public ::testing::Test
    {
    protected:
        static double Price(const OptionParameters& parameters, double annualizedVolatility)
        {
            return BlackScholes(parameters, Volatility::FromAnnualized(annualizedVolatility)).Price();
        }

        void ExpectRoundTrip(const OptionParameters& parameters, double annualizedVolatility, double tolerance) const
        {
            const auto recovered = ImpliedVolatility::From(parameters, Price(parameters, annualizedVolatility));

            ASSERT_TRUE(recovered.has_value());
            EXPECT_NEAR(recovered->Annualized(), annualizedVolatility, tolerance);
        }
    };
}

TEST_F(TestImpliedVolatility, recovers_the_volatility_a_price_was_produced_with)
{
    for (auto type : { OptionType::Call, OptionType::Put })
        for (auto volatility : { 0.01, 0.08, 0.2, 0.55, 1.4, 3.0 })
            for (auto timeToExpiry : { 0.02, 0.5, 4.0 })
                ExpectRoundTrip(OptionScenario(100.0, 100.0, 0.03, 0.02, timeToExpiry, type), volatility, 1e-8);
}

TEST_F(TestImpliedVolatility, converges_far_away_from_the_money)
{
    for (auto strike : { 20.0, 60.0, 140.0, 500.0 })
        for (auto type : { OptionType::Call, OptionType::Put })
            ExpectRoundTrip(OptionScenario(100.0, strike, 0.03, 0.02, 0.75, type), 0.3, 1e-5);
}

TEST_F(TestImpliedVolatility, a_price_at_the_lower_no_arbitrage_bound_implies_no_volatility)
{
    const auto parameters = OptionScenario(120.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);
    const auto recovered = ImpliedVolatility::From(parameters, Price(parameters, 0.0));

    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(*recovered, Volatility::Zero());
}

TEST_F(TestImpliedVolatility, reports_no_result_for_a_price_below_the_lower_no_arbitrage_bound)
{
    const auto parameters = OptionScenario(120.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);

    EXPECT_FALSE(ImpliedVolatility::From(parameters, Price(parameters, 0.0) - 0.01).has_value());
    EXPECT_FALSE(ImpliedVolatility::From(parameters, 0.0).has_value());
    EXPECT_FALSE(ImpliedVolatility::From(parameters, -1.0).has_value());
}

TEST_F(TestImpliedVolatility, reports_no_result_for_a_price_above_the_upper_no_arbitrage_bound)
{
    const auto parameters = OptionScenario(100.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);

    EXPECT_FALSE(ImpliedVolatility::From(parameters, parameters.spot.ToDouble()).has_value());
    EXPECT_FALSE(ImpliedVolatility::From(parameters, 1e9).has_value());
}

TEST_F(TestImpliedVolatility, reports_no_result_for_an_expired_option)
{
    const auto parameters = OptionScenario(120.0, 100.0, 0.03, 0.02, 0.0, OptionType::Call);

    EXPECT_FALSE(ImpliedVolatility::From(parameters, 20.0).has_value());
}

TEST_F(TestImpliedVolatility, reports_no_result_for_a_price_that_is_not_a_number)
{
    const auto parameters = OptionScenario(120.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);

    EXPECT_FALSE(ImpliedVolatility::From(parameters, std::numeric_limits<double>::quiet_NaN()).has_value());
    EXPECT_FALSE(ImpliedVolatility::From(parameters, std::numeric_limits<double>::infinity()).has_value());
}

TEST_F(TestImpliedVolatility, reprices_to_the_observed_price_it_was_given)
{
    const auto parameters = OptionScenario(103.0, 100.0, 0.03, 0.02, 0.4, OptionType::Put);
    const double observed = 5.25;
    const auto recovered = ImpliedVolatility::From(parameters, observed);

    ASSERT_TRUE(recovered.has_value());
    EXPECT_NEAR(BlackScholes(parameters, *recovered).Price(), observed, 1e-10);
}
