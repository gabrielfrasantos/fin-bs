#include "source/pricing/Greeks.hpp"
#include "source/pricing/test/OptionScenario.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    using fin::core::Money;
    using fin::core::OptionType;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::pricing::BlackScholes;
    using fin::pricing::Greeks;
    using fin::pricing::OptionParameters;
    using fin::pricing::test::OptionScenario;

    class TestGreeks
        : public ::testing::Test
    {
    protected:
        static constexpr double spotStep = 0.05;
        static constexpr double volatilityStep = 1e-4;
        static constexpr double timeStep = 1e-5;
        static constexpr double rateStep = 1e-6;
        static constexpr double tolerance = 1e-4;

        static double Price(OptionParameters parameters, double annualizedVolatility)
        {
            return BlackScholes(parameters, Volatility::FromAnnualized(annualizedVolatility)).Price();
        }

        static double PriceAtSpot(OptionParameters parameters, double spot, double annualizedVolatility)
        {
            parameters.spot = Money::FromAmount(spot, parameters.spot.GetCurrency());
            return Price(parameters, annualizedVolatility);
        }

        static double PriceAtTime(OptionParameters parameters, double timeToExpiry, double annualizedVolatility)
        {
            parameters.timeToExpiry = timeToExpiry;
            return Price(parameters, annualizedVolatility);
        }

        static double PriceAtRate(OptionParameters parameters, double riskFreeRate, double annualizedVolatility)
        {
            parameters.riskFreeRate = Rate::FromContinuous(riskFreeRate);
            return Price(parameters, annualizedVolatility);
        }

        void ExpectEveryGreekMatchesAFiniteDifference(const OptionParameters& parameters, double annualizedVolatility) const
        {
            const Greeks greeks(parameters, Volatility::FromAnnualized(annualizedVolatility));
            const double spot = parameters.spot.ToDouble();
            const double up = PriceAtSpot(parameters, spot + spotStep, annualizedVolatility);
            const double down = PriceAtSpot(parameters, spot - spotStep, annualizedVolatility);
            const double centre = Price(parameters, annualizedVolatility);

            EXPECT_NEAR(greeks.Delta().Value(), (up - down) / (2.0 * spotStep), tolerance);
            EXPECT_NEAR(greeks.Gamma(), (up - 2.0 * centre + down) / (spotStep * spotStep), tolerance);
            EXPECT_NEAR(greeks.Vega(), (Price(parameters, annualizedVolatility + volatilityStep) - Price(parameters, annualizedVolatility - volatilityStep)) / (2.0 * volatilityStep), tolerance);
            EXPECT_NEAR(greeks.Theta(), -(PriceAtTime(parameters, parameters.timeToExpiry + timeStep, annualizedVolatility) - PriceAtTime(parameters, parameters.timeToExpiry - timeStep, annualizedVolatility)) / (2.0 * timeStep), tolerance);
            EXPECT_NEAR(greeks.Rho(), (PriceAtRate(parameters, parameters.riskFreeRate.Continuous() + rateStep, annualizedVolatility) - PriceAtRate(parameters, parameters.riskFreeRate.Continuous() - rateStep, annualizedVolatility)) / (2.0 * rateStep), tolerance);
        }
    };
}

TEST_F(TestGreeks, every_greek_matches_a_central_difference_of_the_price)
{
    for (auto spot : { 85.0, 100.0, 115.0 })
        for (auto type : { OptionType::Call, OptionType::Put })
            for (auto volatility : { 0.15, 0.4 })
                ExpectEveryGreekMatchesAFiniteDifference(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, type), volatility);
}

TEST_F(TestGreeks, a_call_delta_lies_between_zero_and_one_and_a_put_delta_between_minus_one_and_zero)
{
    for (auto spot : { 10.0, 85.0, 100.0, 115.0, 400.0 })
    {
        const auto call = Greeks(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, OptionType::Call), Volatility::FromAnnualized(0.2)).Delta();
        const auto put = Greeks(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, OptionType::Put), Volatility::FromAnnualized(0.2)).Delta();

        EXPECT_GT(call.Value(), 0.0);
        EXPECT_LT(call.Value(), 1.0);
        EXPECT_LT(put.Value(), 0.0);
        EXPECT_GT(put.Value(), -1.0);
    }
}

TEST_F(TestGreeks, a_call_and_a_put_on_the_same_contract_have_deltas_that_differ_by_the_carried_spot)
{
    const auto call = OptionScenario(105.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call);
    const auto put = OptionScenario(105.0, 100.0, 0.03, 0.02, 0.75, OptionType::Put);
    const auto volatility = Volatility::FromAnnualized(0.2);

    EXPECT_NEAR(Greeks(call, volatility).Delta().Value() - Greeks(put, volatility).Delta().Value(), call.dividendYield.DiscountFactor(call.timeToExpiry), 1e-12);
}

TEST_F(TestGreeks, gamma_and_vega_are_never_negative_and_do_not_depend_on_the_option_type)
{
    for (auto spot : { 60.0, 100.0, 140.0 })
    {
        const auto volatility = Volatility::FromAnnualized(0.2);
        const Greeks call(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, OptionType::Call), volatility);
        const Greeks put(OptionScenario(spot, 100.0, 0.03, 0.02, 0.75, OptionType::Put), volatility);

        EXPECT_GT(call.Gamma(), 0.0);
        EXPECT_GT(call.Vega(), 0.0);
        EXPECT_DOUBLE_EQ(call.Gamma(), put.Gamma());
        EXPECT_DOUBLE_EQ(call.Vega(), put.Vega());
    }
}

TEST_F(TestGreeks, rho_is_positive_for_a_call_and_negative_for_a_put)
{
    const auto volatility = Volatility::FromAnnualized(0.2);

    EXPECT_GT(Greeks(OptionScenario(100.0, 100.0, 0.03, 0.02, 0.75, OptionType::Call), volatility).Rho(), 0.0);
    EXPECT_LT(Greeks(OptionScenario(100.0, 100.0, 0.03, 0.02, 0.75, OptionType::Put), volatility).Rho(), 0.0);
}

TEST_F(TestGreeks, an_option_without_carry_loses_value_as_time_passes)
{
    const auto volatility = Volatility::FromAnnualized(0.2);

    EXPECT_LT(Greeks(OptionScenario(100.0, 100.0, 0.0, 0.0, 0.75, OptionType::Call), volatility).Theta(), 0.0);
    EXPECT_LT(Greeks(OptionScenario(100.0, 100.0, 0.0, 0.0, 0.75, OptionType::Put), volatility).Theta(), 0.0);
}

TEST_F(TestGreeks, an_expired_option_has_no_curvature_and_no_sensitivity_to_volatility)
{
    const Greeks greeks(OptionScenario(100.0, 100.0, 0.03, 0.02, 0.0, OptionType::Call), Volatility::FromAnnualized(0.2));

    EXPECT_EQ(greeks.Gamma(), 0.0);
    EXPECT_EQ(greeks.Vega(), 0.0);
    EXPECT_EQ(greeks.Rho(), 0.0);
}
