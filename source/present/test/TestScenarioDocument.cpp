#include "source/io/ScenarioLoader.hpp"
#include "source/present/ScenarioDocument.hpp"
#include "source/present/test/RunScenario.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
    using fin::io::Formatted;
    using fin::io::ScenarioLoader;
    using fin::present::Describing;
    using fin::present::IsWellFormed;
    using fin::present::ScenarioDocument;
    using fin::present::UnderlyingRow;
    using fin::present::test::TwoNames;
    using ::testing::ElementsAre;

    UnderlyingRow Row(std::string ticker, std::int32_t spotCents)
    {
        return UnderlyingRow{ .ticker = std::move(ticker),
            .spotCents = spotCents,
            .driftBasisPoints = 600,
            .volatilityBasisPoints = 2200,
            .dividendYieldBasisPoints = 180,
            .strikeIncrementCents = 100 };
    }

    ScenarioDocument Edited()
    {
        return ScenarioDocument(TwoNames());
    }

    std::vector<std::string> Tickers(const ScenarioDocument& document)
    {
        std::vector<std::string> tickers;

        for (const auto& row : document.Underlyings())
            tickers.push_back(row.ticker);

        return tickers;
    }

    std::string Offence(const ScenarioDocument& document)
    {
        return document.Offending().has_value() ? document.Offending()->field : std::string();
    }

    TEST(TestScenarioDocument, a_new_document_wheels_nothing_until_an_underlying_is_named)
    {
        ScenarioDocument document;

        EXPECT_FALSE(IsWellFormed(document));
        EXPECT_EQ(Offence(document), "underlyings");

        document.Append(Row("ACME", 9500));

        EXPECT_TRUE(IsWellFormed(document));
    }

    TEST(TestScenarioDocument, carries_the_scenario_it_was_opened_from)
    {
        const auto document = Edited();

        EXPECT_TRUE(IsWellFormed(document));
        EXPECT_EQ(document.Fields().name, "Two names over two cycles");
        EXPECT_EQ(document.Fields().capitalCents, 20000000);
        EXPECT_EQ(document.Fields().targetDeltaBasisPoints, 3000);
        EXPECT_THAT(Tickers(document), ElementsAre("ACME", "BETA"));
    }

    TEST(TestScenarioDocument, refuses_an_underlying_that_costs_nothing_by_name)
    {
        auto document = Edited();
        document.Replace(1, Row("GAMMA", 0));

        EXPECT_EQ(Offence(document), "underlyings[1].spot_cents");
    }

    TEST(TestScenarioDocument, refuses_a_ticker_the_book_already_holds_by_name)
    {
        auto document = Edited();
        document.Append(Row("ACME", 9500));

        EXPECT_EQ(Offence(document), "underlyings[0].ticker");
        EXPECT_EQ(Describing(*document.Offending()), "underlyings[0].ticker names an underlying the book already holds");
    }

    TEST(TestScenarioDocument, refuses_a_book_of_no_underlyings_by_name)
    {
        auto document = Edited();
        document.Remove(1);
        document.Remove(0);

        EXPECT_EQ(Offence(document), "underlyings");
    }

    TEST(TestScenarioDocument, refuses_a_delta_outside_the_range_a_delta_lives_in_by_name)
    {
        auto document = Edited();
        auto fields = document.Fields();
        fields.targetDeltaBasisPoints = 15000;
        document.Amend(fields);

        EXPECT_EQ(Offence(document), "target_delta_basis_points");
        EXPECT_EQ(Describing(*document.Offending()), "target_delta_basis_points lies outside 1 to 9999");
    }

    TEST(TestScenarioDocument, refuses_an_observation_that_falls_before_the_cycle_it_belongs_to)
    {
        auto document = Edited();
        auto fields = document.Fields();
        fields.observationDaysBeforeExpiry = fields.daysPerCycle;
        document.Amend(fields);

        EXPECT_EQ(Offence(document), "observation_days_before_expiry");
    }

    TEST(TestScenarioDocument, moving_a_row_changes_the_order_and_nothing_else)
    {
        auto document = Edited();
        const auto before = document.Fields();

        document.Move(1, 0);

        EXPECT_THAT(Tickers(document), ElementsAre("BETA", "ACME"));
        EXPECT_EQ(document.Fields(), before);
        EXPECT_EQ(document.Underlyings()[0], ScenarioDocument(TwoNames()).Underlyings()[1]);
        EXPECT_EQ(document.Underlyings()[1], ScenarioDocument(TwoNames()).Underlyings()[0]);
    }

    TEST(TestScenarioDocument, adopting_an_estimate_changes_exactly_the_volatility_and_the_drift)
    {
        auto document = Edited();
        const auto before = document.Underlyings()[0];

        document.Adopt(0, 3300, -250);

        const auto after = document.Underlyings()[0];

        EXPECT_EQ(after.volatilityBasisPoints, 3300);
        EXPECT_EQ(after.driftBasisPoints, -250);
        EXPECT_EQ(after.ticker, before.ticker);
        EXPECT_EQ(after.spotCents, before.spotCents);
        EXPECT_EQ(after.dividendYieldBasisPoints, before.dividendYieldBasisPoints);
        EXPECT_EQ(after.strikeIncrementCents, before.strikeIncrementCents);
    }

    TEST(TestScenarioDocument, is_the_same_document_after_it_is_written_and_read_back)
    {
        const auto document = Edited();
        const ScenarioLoader loader({ Formatted(document.Prepared()) });

        ASSERT_TRUE(loader.Loaded().has_value());
        EXPECT_EQ(ScenarioDocument(*loader.Loaded()), document);
    }

    TEST(TestScenarioDocument, keeps_the_order_of_the_underlyings_through_a_write_and_a_read)
    {
        auto document = Edited();
        document.Move(1, 0);

        const ScenarioLoader loader({ Formatted(document.Prepared()) });

        ASSERT_TRUE(loader.Loaded().has_value());
        EXPECT_THAT(Tickers(ScenarioDocument(*loader.Loaded())), ElementsAre("BETA", "ACME"));
    }
}
