#include "source/io/ScenarioLoader.hpp"
#include <cstdint>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::io::Formatted;
    using fin::io::Scenario;
    using fin::io::ScenarioLoader;
    using ::testing::ElementsAre;
    using ::testing::HasSubstr;

    std::string Published()
    {
        std::ifstream file(std::string(FIN_BS_CONFIG_DIRECTORY) + "/scenarios/example.json");
        std::ostringstream contents;
        contents << file.rdbuf();

        return contents.str();
    }

    std::vector<std::string> PublishedScenario()
    {
        return { Published() };
    }

    std::vector<std::string> Replacing(std::string_view what, std::string_view with)
    {
        auto text = Published();
        const auto at = text.find(what);

        return { text.replace(at, what.size(), with) };
    }

    std::string Row(std::string_view ticker, std::int32_t spotCents)
    {
        return R"({ "ticker": ")" + std::string(ticker) + R"(", "spot_cents": )" + std::to_string(spotCents) +
               R"(, "drift_basis_points": 600, "volatility_basis_points": 2200, "implied_volatility_basis_points": 2400, "dividend_yield_basis_points": 180, "dividend_withholding_basis_points": 1500, "strike_increment_cents": 100 })";
    }

    std::vector<std::string> WithUnderlyings(const std::string& rows)
    {
        auto text = Published();
        text.resize(text.find(R"("underlyings")"));

        return { text + R"("underlyings": [)" + rows + "] }" };
    }

    constexpr std::string_view calibrationBlock = R"("calibration": {
        "price_histories": "../data/prices",
        "window_observations": 252,
        "volatility_estimator": "exponentially_weighted",
        "half_life_observations": 60,
        "drift_estimator": "least_squares",
        "implied_premium_basis_points": 200
    },
    )";

    std::vector<std::string> Calibrated(std::string_view block = calibrationBlock)
    {
        auto text = Published();
        const auto at = text.find(R"("underlyings")");

        return { text.insert(at, block) };
    }

    std::vector<std::string> CalibratedReplacing(std::string_view what, std::string_view with)
    {
        auto block = std::string(calibrationBlock);
        const auto at = block.find(what);

        return Calibrated(block.replace(at, what.size(), with));
    }

    std::vector<std::string> Tickers(const Scenario& scenario)
    {
        std::vector<std::string> tickers;

        for (const auto& underlying : scenario.terms.underlyings)
            tickers.push_back(underlying.underlying);

        return tickers;
    }

    TEST(TestScenarioLoader, reads_the_published_scenario)
    {
        const ScenarioLoader loader(PublishedScenario());

        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();
        EXPECT_EQ(loader.Loaded()->capital, Money::FromCents(20000000, Currency::Eur));
        EXPECT_EQ(loader.Loaded()->cashBufferBasisPoints, 500);
        EXPECT_EQ(loader.Loaded()->paths, 256);
        EXPECT_EQ(loader.Loaded()->seed, 20250811u);
        EXPECT_EQ(loader.Loaded()->taxConfiguration, "tax/nl_2025.json");
        EXPECT_EQ(loader.Loaded()->brokerConfiguration, "broker/degiro.json");
        EXPECT_EQ(loader.Loaded()->terms.start, Date::FromYearMonthDay(2026, 1, 5));
        EXPECT_EQ(loader.Loaded()->terms.cycles, 6);
        EXPECT_EQ(loader.Loaded()->terms.daysPerCycle, 182);
        EXPECT_EQ(loader.Loaded()->terms.observationDaysBeforeExpiry, 14);
        EXPECT_DOUBLE_EQ(loader.Loaded()->terms.targetDelta.Value(), 0.30);
        EXPECT_DOUBLE_EQ(loader.Loaded()->terms.riskFreeRate.Continuous(), 0.025);
        EXPECT_EQ(loader.Loaded()->triggers.daysToExpiry, 7);
        EXPECT_EQ(loader.Loaded()->triggers.profitCaptureBasisPoints, 8000);
        EXPECT_DOUBLE_EQ(loader.Loaded()->triggers.deltaLimit.Value(), 0.50);
        ASSERT_TRUE(loader.Loaded()->terms.assessment.has_value());
        EXPECT_EQ(loader.Loaded()->terms.assessment->month, 1u);
        EXPECT_EQ(loader.Loaded()->terms.assessment->day, 1u);
        EXPECT_THAT(Tickers(*loader.Loaded()), ElementsAre("AEX", "ASML", "SHELL"));
    }

    TEST(TestScenarioLoader, reads_every_underlying_of_the_published_scenario)
    {
        const ScenarioLoader loader(PublishedScenario());

        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();

        const auto& second = loader.Loaded()->terms.underlyings[1];
        EXPECT_EQ(second.underlying, "ASML");
        EXPECT_EQ(second.initialPrice, Money::FromCents(68000, Currency::Eur));
        EXPECT_DOUBLE_EQ(second.drift.Continuous(), 0.09);
        EXPECT_DOUBLE_EQ(second.volatility.Annualized(), 0.34);
        EXPECT_DOUBLE_EQ(second.impliedVolatility.Annualized(), 0.34);
        EXPECT_DOUBLE_EQ(second.dividendYield.Continuous(), 0.009);
        EXPECT_EQ(second.dividendWithholdingBasisPoints, 1500);
        EXPECT_EQ(second.strikeIncrement, Money::FromCents(500, Currency::Eur));
    }

    TEST(TestScenarioLoader, writes_a_scenario_that_reads_back_as_the_same_scenario)
    {
        const ScenarioLoader first(PublishedScenario());
        ASSERT_TRUE(first.Loaded().has_value()) << first.Failure();

        const ScenarioLoader again({ Formatted(*first.Loaded()) });

        ASSERT_TRUE(again.Loaded().has_value()) << again.Failure();
        EXPECT_EQ(*again.Loaded(), *first.Loaded());
    }

    TEST(TestScenarioLoader, keeps_the_order_the_underlyings_are_listed_in)
    {
        const ScenarioLoader loader(WithUnderlyings(Row("THIRD", 3000) + "," + Row("FIRST", 1000) + "," + Row("SECOND", 2000)));
        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();

        const ScenarioLoader again({ Formatted(*loader.Loaded()) });

        ASSERT_TRUE(again.Loaded().has_value()) << again.Failure();
        EXPECT_THAT(Tickers(*loader.Loaded()), ElementsAre("THIRD", "FIRST", "SECOND"));
        EXPECT_THAT(Tickers(*again.Loaded()), ElementsAre("THIRD", "FIRST", "SECOND"));
        EXPECT_EQ(*again.Loaded(), *loader.Loaded());
    }

    TEST(TestScenarioLoader, writes_a_scenario_that_keeps_no_assessment)
    {
        const ScenarioLoader loader(Replacing(R"("assessment": {
        "month": 1,
        "day": 1
    },)",
            ""));
        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();
        EXPECT_FALSE(loader.Loaded()->terms.assessment.has_value());

        const ScenarioLoader again({ Formatted(*loader.Loaded()) });

        ASSERT_TRUE(again.Loaded().has_value()) << again.Failure();
        EXPECT_EQ(*again.Loaded(), *loader.Loaded());
    }

    TEST(TestScenarioLoader, refuses_a_book_of_no_underlyings)
    {
        const ScenarioLoader loader(WithUnderlyings(""));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("underlyings"));
        EXPECT_THAT(loader.Failure(), HasSubstr("names no underlying"));
    }

    TEST(TestScenarioLoader, refuses_a_ticker_that_names_two_underlyings)
    {
        const ScenarioLoader loader(WithUnderlyings(Row("AEX", 9500) + "," + Row("AEX", 9600)));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("AEX"));
        EXPECT_THAT(loader.Failure(), HasSubstr("two underlyings"));
    }

    TEST(TestScenarioLoader, refuses_a_field_written_as_the_wrong_kind_of_value)
    {
        const ScenarioLoader loader(Replacing(R"("capital_cents": 20000000)", R"("capital_cents": "two hundred thousand")"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("capital_cents"));
    }

    TEST(TestScenarioLoader, refuses_a_field_that_is_not_there)
    {
        const ScenarioLoader loader(Replacing(R"("target_delta_basis_points": 3000,)", ""));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("target_delta_basis_points"));
    }

    TEST(TestScenarioLoader, refuses_a_field_of_an_underlying_that_is_not_there)
    {
        const ScenarioLoader loader(Replacing(R"("volatility_basis_points": 3400,)", ""));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("volatility_basis_points"));
    }

    TEST(TestScenarioLoader, refuses_a_field_outside_the_range_it_allows)
    {
        const ScenarioLoader loader(Replacing(R"("days_per_cycle": 182)", R"("days_per_cycle": 400)"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("days_per_cycle"));
        EXPECT_THAT(loader.Failure(), HasSubstr("outside the range"));
    }

    TEST(TestScenarioLoader, refuses_an_amount_smaller_than_the_field_allows)
    {
        const ScenarioLoader loader(Replacing(R"("capital_cents": 20000000)", R"("capital_cents": 0)"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("capital_cents"));
    }

    TEST(TestScenarioLoader, refuses_an_observation_that_falls_before_a_cycle_opens)
    {
        const ScenarioLoader loader(Replacing(R"("observation_days_before_expiry": 14)", R"("observation_days_before_expiry": 182)"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("observation_days_before_expiry"));
    }

    TEST(TestScenarioLoader, refuses_a_start_that_is_not_a_day_of_the_calendar)
    {
        const ScenarioLoader loader(Replacing(R"("start_date": "2026-01-05")", R"("start_date": "2026-02-30")"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("start_date"));
    }

    TEST(TestScenarioLoader, refuses_an_assessment_on_a_day_that_not_every_year_holds)
    {
        const ScenarioLoader loader(Replacing(R"("month": 1,
        "day": 1)",
            R"("month": 2,
        "day": 29)"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("assessment"));
    }

    TEST(TestScenarioLoader, refuses_a_currency_it_does_not_hold)
    {
        const ScenarioLoader loader(Replacing(R"("currency": "EUR")", R"("currency": "GBP")"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("GBP"));
    }
}

namespace
{
    TEST(TestScenarioLoader, refuses_an_underlying_without_an_implied_volatility_or_a_withholding_rate)
    {
        const ScenarioLoader unpriced(Replacing(R"("implied_volatility_basis_points": 3400,)", ""));
        const ScenarioLoader unwithheld(Replacing(R"("dividend_withholding_basis_points": 1500,
            "strike_increment_cents": 500)",
            R"("strike_increment_cents": 500)"));

        EXPECT_FALSE(unpriced.Loaded().has_value());
        EXPECT_THAT(unpriced.Failure(), HasSubstr("implied_volatility_basis_points"));
        EXPECT_FALSE(unwithheld.Loaded().has_value());
        EXPECT_THAT(unwithheld.Failure(), HasSubstr("dividend_withholding_basis_points"));
    }
}

namespace
{
    using fin::sim::DriftEstimator;
    using fin::sim::VolatilityEstimator;

    TEST(TestScenarioLoader, a_scenario_without_a_calibration_block_states_its_terms_outright)
    {
        const ScenarioLoader loader(PublishedScenario());

        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();
        EXPECT_FALSE(loader.Loaded()->calibration.has_value());
    }

    TEST(TestScenarioLoader, reads_a_calibration_block_and_writes_it_back_as_it_was)
    {
        const ScenarioLoader loader(Calibrated());

        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();
        ASSERT_TRUE(loader.Loaded()->calibration.has_value());
        EXPECT_EQ(loader.Loaded()->calibration->priceHistories, "../data/prices");
        EXPECT_EQ(loader.Loaded()->calibration->rule, (fin::sim::CalibrationRule{ 252, VolatilityEstimator::exponentiallyWeighted, 60, DriftEstimator::leastSquares, 200 }));

        const ScenarioLoader again({ Formatted(*loader.Loaded()) });

        ASSERT_TRUE(again.Loaded().has_value()) << again.Failure();
        EXPECT_EQ(*again.Loaded(), *loader.Loaded());
    }

    TEST(TestScenarioLoader, reads_the_other_estimators_by_name)
    {
        const ScenarioLoader loader(CalibratedReplacing(R"("volatility_estimator": "exponentially_weighted",
        "half_life_observations": 60,
        "drift_estimator": "least_squares")",
            R"("volatility_estimator": "historical",
        "half_life_observations": 60,
        "drift_estimator": "stated")"));

        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();
        EXPECT_EQ(loader.Loaded()->calibration->rule.volatility, VolatilityEstimator::historical);
        EXPECT_EQ(loader.Loaded()->calibration->rule.drift, DriftEstimator::stated);
    }

    TEST(TestScenarioLoader, refuses_a_calibration_block_that_leaves_a_key_out)
    {
        const ScenarioLoader loader(CalibratedReplacing(R"("window_observations": 252,)", ""));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("window_observations"));
    }

    TEST(TestScenarioLoader, refuses_an_estimator_it_does_not_hold)
    {
        const ScenarioLoader loader(CalibratedReplacing(R"("exponentially_weighted")", R"("garch")"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("garch"));
    }

    TEST(TestScenarioLoader, refuses_a_half_life_longer_than_the_window)
    {
        const ScenarioLoader loader(CalibratedReplacing(R"("half_life_observations": 60)", R"("half_life_observations": 253)"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("half_life_observations"));
    }

    TEST(TestScenarioLoader, refuses_a_window_shorter_than_a_volatility_can_be_read_from)
    {
        const ScenarioLoader loader(CalibratedReplacing(R"("window_observations": 252)", R"("window_observations": 19)"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("window_observations"));
    }

    TEST(TestScenarioLoader, refuses_a_folder_that_names_nothing)
    {
        const ScenarioLoader loader(CalibratedReplacing(R"("../data/prices")", R"("")"));

        EXPECT_FALSE(loader.Loaded().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("price_histories"));
    }

    TEST(TestScenarioLoader, reads_the_published_backtest_scenario)
    {
        std::ifstream file(std::string(FIN_BS_CONFIG_DIRECTORY) + "/scenarios/benelux-backtest.json");
        std::ostringstream contents;
        contents << file.rdbuf();

        const ScenarioLoader loader({ contents.str() });

        ASSERT_TRUE(loader.Loaded().has_value()) << loader.Failure();
        ASSERT_TRUE(loader.Loaded()->calibration.has_value());
        EXPECT_EQ(loader.Loaded()->terms.cycles, 2);
        EXPECT_EQ(loader.Loaded()->terms.underlyings.size(), 10u);
    }
}
