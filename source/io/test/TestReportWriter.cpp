#include "source/io/ReportWriter.hpp"
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Delta;
    using fin::core::Money;
    using fin::costs::BrokerFees;
    using fin::costs::FeeSchedule;
    using fin::io::CycleReport;
    using fin::io::LedgerReport;
    using fin::strategy::Book;
    using fin::strategy::BookAssessment;
    using fin::strategy::BookPeriod;
    using fin::strategy::BookRow;
    using fin::strategy::Portfolio;
    using fin::strategy::ProjectionPeriod;
    using fin::strategy::ReinvestmentPolicy;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;
    using fin::strategy::SpotPrice;
    using fin::tax::Box3Parameters;
    using fin::tax::Box3TaxModel;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    BrokerFees Fees()
    {
        return BrokerFees(FeeSchedule{ .optionFeePerContract = Euro(0.75),
            .shareCommissionPerTrade = Euro(2.00),
            .handlingFeePerTrade = Euro(1.00),
            .connectivityFeeCapPerExchangePerYear = Euro(2.50),
            .connectivityBasisPoints = 25,
            .currencyHandlingBasisPoints = 25,
            .debitInterestBasisPoints = 690 });
    }

    Box3TaxModel Rules()
    {
        return Box3TaxModel(Box3Parameters{ .taxYear = 2025,
            .taxFreeAllowance = Euro(57684.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 });
    }

    ProjectionPeriod Period(const Date& opened, const Date& expiry, double strike, double premiumPerShare, double spotAtExpiry)
    {
        return { .opened = opened,
            .expiry = expiry,
            .strike = Euro(strike),
            .premiumPerShare = premiumPerShare,
            .spotAtExpiry = Euro(spotAtExpiry),
            .observed = std::nullopt,
            .assessed = std::nullopt };
    }

    struct Run
    {
        std::vector<BookRow> rows;
        fin::strategy::CompoundingLedger ledger;
    };

    Run TwoNamesOverTwoCycles()
    {
        Book book({ "ACME", "BETA" },
            Portfolio::Funded(Euro(200000.00)),
            ReinvestmentPolicy::WithCashBuffer(1000),
            RollRules(RollTriggers{ .daysToExpiry = 0, .profitCaptureBasisPoints = 10000, .deltaLimit = Delta::FromValue(1.0) }),
            Fees(),
            Rules());

        std::vector<BookRow> rows;

        const std::vector<BookPeriod> first{
            { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00) },
            { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 50.00, 1.00, 45.00) }
        };

        for (const auto& row : book.Over(first))
            rows.push_back(row);

        book.Assess(BookAssessment{ On(2026, 1, 1), { SpotPrice{ "ACME", Euro(45.00) }, SpotPrice{ "BETA", Euro(45.00) } } });

        const std::vector<BookPeriod> second{
            { "ACME", Period(On(2025, 7, 18), On(2026, 1, 16), 44.00, 0.90, 48.00) },
            { "BETA", Period(On(2025, 7, 18), On(2026, 1, 16), 46.00, 1.10, 48.00) }
        };

        for (const auto& row : book.Over(second))
            rows.push_back(row);

        return Run{ std::move(rows), book.Ledger() };
    }

    TEST(TestReportWriter, writes_a_row_of_the_cycle_table_for_every_underlying_of_every_cycle)
    {
        const auto run = TwoNamesOverTwoCycles();

        EXPECT_EQ(CycleReport(run.rows),
            "underlying,date,state,contracts,premium,fees,assignment,dividend,tax,cash,shares,equity\n"
            "ACME,2025-07-18,cash,45,3600.00,33.75,0.00,0.00,0.00,203963.25,0,201963.25\n"
            "BETA,2025-07-18,holding shares,4,400.00,6.00,-20000.00,0.00,0.00,183960.25,400,201960.25\n"
            "ACME,2026-01-16,cash,37,3330.00,27.75,0.00,0.00,0.00,186779.15,0,205179.15\n"
            "BETA,2026-01-16,cash,4,440.00,6.00,18400.00,0.00,0.00,205176.15,0,205176.15\n");
    }

    TEST(TestReportWriter, writes_a_row_of_the_ledger_for_every_cashflow)
    {
        const auto run = TwoNamesOverTwoCycles();

        EXPECT_EQ(LedgerReport(run.ledger),
            "date,kind,amount\n"
            "2025-01-03,premium,3600.00\n"
            "2025-01-03,fee,-33.75\n"
            "2025-01-03,premium,400.00\n"
            "2025-01-03,fee,-3.00\n"
            "2025-07-18,assignment,-20000.00\n"
            "2025-07-18,fee,-3.00\n"
            "2025-07-18,premium,440.00\n"
            "2025-07-18,fee,-3.00\n"
            "2025-07-18,premium,3330.00\n"
            "2025-07-18,fee,-27.75\n"
            "2026-01-01,tax,-920.35\n"
            "2026-01-16,assignment,18400.00\n"
            "2026-01-16,fee,-3.00\n");
    }

    TEST(TestReportWriter, writes_an_empty_table_of_no_rows)
    {
        EXPECT_EQ(CycleReport({}), "underlying,date,state,contracts,premium,fees,assignment,dividend,tax,cash,shares,equity\n");
        EXPECT_EQ(LedgerReport(fin::strategy::CompoundingLedger(Currency::Eur)), "date,kind,amount\n");
    }
}
