#include "source/estimation/MarketParameters.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "source/sim/Calibration.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include <cmath>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::estimation::MarketParameters;
    using fin::estimation::OrdinaryLeastSquaresFit;
    using fin::estimation::PriceSeries;
    using fin::sim::CalibratedEstimate;
    using fin::sim::CalibrationRule;
    using fin::sim::Calibrator;
    using fin::sim::DriftEstimator;
    using fin::sim::GbmParameters;
    using fin::sim::GbmPathGenerator;
    using fin::sim::IsWellFormed;
    using fin::sim::Rng;
    using fin::sim::UnderlyingTerms;
    using fin::sim::VolatilityEstimator;
    using ::testing::HasSubstr;

    constexpr auto annualVolatility = 0.25;
    constexpr auto window = 252;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    std::vector<Date> BusinessDays(const Date& start, std::size_t observations)
    {
        std::vector<Date> dates;
        auto date = start;

        while (dates.size() != observations)
        {
            if (!date.IsWeekend())
                dates.push_back(date);

            date = date.AddDays(1);
        }

        return dates;
    }

    PriceSeries ThreeYears()
    {
        Rng rng(7u);
        const GbmPathGenerator generator(GbmParameters{ .initialPrice = Euro(50.00),
            .drift = Rate::FromContinuous(0.08),
            .volatility = Volatility::FromAnnualized(annualVolatility),
            .convention = DayCountConvention::Act252 });

        return generator.Along(BusinessDays(On(2023, 1, 2), 3 * 252), rng);
    }

    UnderlyingTerms Stated()
    {
        return { .underlying = "ACME",
            .initialPrice = Euro(50.00),
            .drift = Rate::FromContinuous(0.05),
            .volatility = Volatility::FromAnnualized(0.30),
            .impliedVolatility = Volatility::FromAnnualized(0.32),
            .dividendYield = Rate::FromContinuous(0.02),
            .dividendWithholdingBasisPoints = 1500,
            .strikeIncrement = Euro(1.00) };
    }

    CalibrationRule Historical(std::int32_t premiumBasisPoints = 200)
    {
        return { .windowObservations = window, .volatility = VolatilityEstimator::historical, .halfLifeObservations = 60, .drift = DriftEstimator::leastSquares, .impliedPremiumBasisPoints = premiumBasisPoints };
    }

    CalibrationRule Weighted()
    {
        return { .windowObservations = window, .volatility = VolatilityEstimator::exponentiallyWeighted, .halfLifeObservations = 60, .drift = DriftEstimator::stated, .impliedPremiumBasisPoints = 0 };
    }

    PriceSeries Window(const PriceSeries& history, std::size_t end)
    {
        const auto dates = history.Dates().subspan(end - window, window);
        const auto closes = history.Closes().subspan(end - window, window);

        return PriceSeries::From(std::vector<Date>(dates.begin(), dates.end()), std::vector<Money>(closes.begin(), closes.end()), history.Convention());
    }
}

TEST(TestCalibration, recovers_the_volatility_a_history_was_drawn_with)
{
    const auto history = ThreeYears();
    const Calibrator calibrator(Historical(), history, On(2025, 1, 6), Stated());

    ASSERT_TRUE(calibrator.Estimate().has_value()) << calibrator.Failure();
    EXPECT_NEAR(calibrator.Estimate()->volatility.Annualized(), annualVolatility, 0.05);
    EXPECT_EQ(calibrator.Estimate()->observations, window);
}

TEST(TestCalibration, weights_recent_closes_and_still_recovers_a_constant_volatility)
{
    const auto history = ThreeYears();
    const Calibrator calibrator(Weighted(), history, On(2025, 1, 6), Stated());

    ASSERT_TRUE(calibrator.Estimate().has_value()) << calibrator.Failure();
    EXPECT_NEAR(calibrator.Estimate()->volatility.Annualized(), annualVolatility, 0.07);
}

TEST(TestCalibration, uses_only_the_closes_strictly_before_the_date_it_is_asked_about)
{
    const auto history = ThreeYears();
    const auto on = history.Dates()[600];
    const Calibrator calibrator(Historical(), history, on, Stated());

    ASSERT_TRUE(calibrator.Estimate().has_value());
    EXPECT_EQ(calibrator.Estimate()->windowEnd, history.Dates()[599]);

    const auto parameters = MarketParameters::From(Window(history, 600), Rate::Zero(), OrdinaryLeastSquaresFit());

    ASSERT_TRUE(parameters.has_value());
    EXPECT_EQ(calibrator.Estimate()->volatility, parameters->Volatility());
    EXPECT_EQ(calibrator.Estimate()->drift, parameters->Drift());
}

TEST(TestCalibration, keeps_the_stated_drift_when_the_rule_says_so)
{
    const auto history = ThreeYears();
    const Calibrator calibrator(Weighted(), history, On(2025, 1, 6), Stated());

    ASSERT_TRUE(calibrator.Estimate().has_value());
    EXPECT_EQ(calibrator.Estimate()->drift, Stated().drift);
}

TEST(TestCalibration, adds_the_premium_to_the_realised_volatility_for_the_implied_one)
{
    const auto history = ThreeYears();
    const Calibrator calibrator(Historical(300), history, On(2025, 1, 6), Stated());

    ASSERT_TRUE(calibrator.Estimate().has_value());
    EXPECT_NEAR(calibrator.Estimate()->impliedVolatility.Annualized(), calibrator.Estimate()->volatility.Annualized() + 0.03, 1e-12);
}

TEST(TestCalibration, refuses_a_window_the_history_cannot_fill_and_says_how_short_it_is)
{
    const auto history = ThreeYears();
    const Calibrator calibrator(Historical(), history, history.Dates()[100], Stated());

    EXPECT_FALSE(calibrator.Estimate().has_value());
    EXPECT_THAT(calibrator.Failure(), HasSubstr("holds 100 closes before"));
    EXPECT_THAT(calibrator.Failure(), HasSubstr("needs 252"));
}

TEST(TestCalibration, refuses_a_premium_that_takes_the_implied_volatility_to_nothing)
{
    const auto history = ThreeYears();
    const Calibrator calibrator(Historical(-10000), history, On(2025, 1, 6), Stated());

    EXPECT_FALSE(calibrator.Estimate().has_value());
    EXPECT_THAT(calibrator.Failure(), HasSubstr("to nothing"));
}

TEST(TestCalibration, refuses_closes_that_do_not_move)
{
    const auto dates = BusinessDays(On(2024, 1, 2), 300);
    const auto history = PriceSeries::From(dates, std::vector<Money>(dates.size(), Euro(40.00)), DayCountConvention::Act252);
    const Calibrator calibrator(Historical(), history, dates.back(), Stated());

    EXPECT_FALSE(calibrator.Estimate().has_value());
    EXPECT_THAT(calibrator.Failure(), HasSubstr("do not move"));
}

TEST(TestCalibration, a_calibrated_underlying_keeps_everything_but_its_drift_and_volatilities)
{
    const CalibratedEstimate estimate{ .windowEnd = On(2025, 1, 3), .observations = window, .drift = Rate::FromContinuous(0.09), .volatility = Volatility::FromAnnualized(0.21), .impliedVolatility = Volatility::FromAnnualized(0.23) };
    const auto calibrated = fin::sim::Calibrated(Stated(), estimate);

    EXPECT_EQ(calibrated.drift, Rate::FromContinuous(0.09));
    EXPECT_EQ(calibrated.volatility, Volatility::FromAnnualized(0.21));
    EXPECT_EQ(calibrated.impliedVolatility, Volatility::FromAnnualized(0.23));
    EXPECT_EQ(calibrated.underlying, "ACME");
    EXPECT_EQ(calibrated.initialPrice, Euro(50.00));
    EXPECT_EQ(calibrated.dividendYield, Stated().dividendYield);
    EXPECT_EQ(calibrated.dividendWithholdingBasisPoints, 1500);
    EXPECT_EQ(calibrated.strikeIncrement, Euro(1.00));
}

TEST(TestCalibration, a_rule_is_well_formed_within_its_bounds)
{
    EXPECT_TRUE(IsWellFormed(Historical()));
    EXPECT_FALSE(IsWellFormed(CalibrationRule{ 19, VolatilityEstimator::historical, 5, DriftEstimator::leastSquares, 0 }));
    EXPECT_FALSE(IsWellFormed(CalibrationRule{ 5001, VolatilityEstimator::historical, 5, DriftEstimator::leastSquares, 0 }));
    EXPECT_FALSE(IsWellFormed(CalibrationRule{ 100, VolatilityEstimator::exponentiallyWeighted, 0, DriftEstimator::leastSquares, 0 }));
    EXPECT_FALSE(IsWellFormed(CalibrationRule{ 100, VolatilityEstimator::exponentiallyWeighted, 101, DriftEstimator::leastSquares, 0 }));
    EXPECT_FALSE(IsWellFormed(CalibrationRule{ 100, VolatilityEstimator::historical, 5, DriftEstimator::leastSquares, 10001 }));
    EXPECT_FALSE(IsWellFormed(CalibrationRule{ 100, VolatilityEstimator::historical, 5, DriftEstimator::leastSquares, -10001 }));
}
