#include "source/present/ResultTable.hpp"
#include "source/present/test/RunScenario.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;
    using fin::present::CycleTable;
    using fin::present::LedgerTable;
    using fin::present::OutcomeTable;
    using fin::present::ResultTable;
    using fin::present::test::Euro;
    using fin::sim::OutcomeDistribution;
    using fin::sim::PathOutcome;
    using fin::strategy::BookRow;
    using fin::strategy::CashflowKind;
    using fin::strategy::CompoundingLedger;
    using fin::strategy::ProjectionRow;
    using fin::strategy::WheelState;
    using ::testing::Contains;
    using ::testing::ElementsAre;

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    std::vector<BookRow> Rows()
    {
        return { BookRow{ "ACME", ProjectionRow{ .on = On(2026, 7, 6), .state = WheelState::Cash, .contracts = 45, .premium = Euro(3600.00), .fees = Euro(-33.75), .assignment = Money::Zero(Currency::Eur), .dividend = Money::Zero(Currency::Eur), .tax = Money::Zero(Currency::Eur), .cash = Euro(203963.25), .shares = 0, .equity = Euro(201963.25) } },
            BookRow{ "BETA", ProjectionRow{ .on = On(2026, 7, 6), .state = WheelState::HoldingShares, .contracts = 4, .premium = Euro(400.00), .fees = Euro(-6.00), .assignment = Euro(-20000.00), .dividend = Euro(12.75), .tax = Money::Zero(Currency::Eur), .cash = Euro(183960.25), .shares = 400, .equity = Euro(201960.25) } } };
    }

    OutcomeDistribution Outcomes()
    {
        return OutcomeDistribution::Of({ PathOutcome{ .terminalEquity = Euro(210000.00), .growthRate = 0.05, .positions = 4, .assignments = 1, .maximumDrawdown = 0.125, .equityByCycle = { Euro(200000.00), Euro(210000.00) } },
            PathOutcome{ .terminalEquity = Euro(190000.00), .growthRate = -0.05, .positions = 4, .assignments = 3, .maximumDrawdown = 0.25, .equityByCycle = { Euro(200000.00), Euro(190000.00) } } });
    }

    TEST(TestResultTable, names_a_column_for_every_field_of_a_cycle)
    {
        EXPECT_THAT(CycleTable({}).headings, ElementsAre("underlying", "date", "state", "contracts", "premium", "fees", "assignment", "dividend", "tax", "cash", "shares", "equity"));
        EXPECT_TRUE(CycleTable({}).rows.empty());
    }

    TEST(TestResultTable, writes_money_to_the_cent_and_never_as_a_float)
    {
        const auto rows = Rows();

        EXPECT_THAT(CycleTable(rows).rows, ElementsAre(ElementsAre("ACME", "2026-07-06", "cash", "45", "3600.00", "-33.75", "0.00", "0.00", "0.00", "203963.25", "0", "201963.25"),
                                               ElementsAre("BETA", "2026-07-06", "holding shares", "4", "400.00", "-6.00", "-20000.00", "12.75", "0.00", "183960.25", "400", "201960.25")));
    }

    TEST(TestResultTable, writes_a_row_of_the_ledger_for_every_cashflow)
    {
        CompoundingLedger ledger(Currency::Eur);
        ledger.Post(On(2026, 1, 5), CashflowKind::Premium, Euro(3600.00));
        ledger.Post(On(2026, 1, 5), CashflowKind::Fee, Euro(-33.75));

        EXPECT_THAT(LedgerTable(ledger).headings, ElementsAre("date", "kind", "amount"));
        EXPECT_THAT(LedgerTable(ledger).rows, ElementsAre(ElementsAre("2026-01-05", "premium", "3600.00"), ElementsAre("2026-01-05", "fee", "-33.75")));
    }

    TEST(TestResultTable, writes_a_rate_as_a_percentage_of_the_hundredth)
    {
        const auto table = OutcomeTable(Outcomes());

        EXPECT_THAT(table.rows, Contains(ElementsAre("paths", "2")));
        EXPECT_THAT(table.rows, Contains(ElementsAre("equity at the median", "190000.00")));
        EXPECT_THAT(table.rows, Contains(ElementsAre("mean equity", "200000.00")));
        EXPECT_THAT(table.rows, Contains(ElementsAre("worst drawdown", "25.00%")));
        EXPECT_THAT(table.rows, Contains(ElementsAre("assignments per option written", "50.00%")));
        EXPECT_THAT(table.rows, Contains(ElementsAre("mean growth a year", "0.00%")));
        EXPECT_THAT(table.rows, Contains(ElementsAre("paths that borrowed", "0.00%")));
    }
}

namespace
{
    TEST(TestResultTable, writes_an_estimate_per_cycle_and_underlying_in_percent)
    {
        const std::vector<fin::present::CalibrationRow> rows{ { 0, "ACME", fin::sim::CalibratedEstimate{ .windowEnd = fin::core::Date::FromYearMonthDay(2026, 1, 2), .observations = 252, .drift = fin::core::Rate::FromContinuous(0.08), .volatility = fin::core::Volatility::FromAnnualized(0.25), .impliedVolatility = fin::core::Volatility::FromAnnualized(0.27) } } };
        const auto table = fin::present::CalibrationTable(rows);

        EXPECT_THAT(table.headings, ::testing::ElementsAre("cycle", "underlying", "window end", "closes", "drift", "volatility", "implied"));
        ASSERT_EQ(table.rows.size(), 1u);
        EXPECT_THAT(table.rows[0], ::testing::ElementsAre("1", "ACME", "2026-01-02", "252", "8.00%", "25.00%", "27.00%"));
    }
}
