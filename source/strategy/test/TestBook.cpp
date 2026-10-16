#include "source/strategy/Book.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Delta;
    using fin::core::Money;
    using fin::costs::BrokerFees;
    using fin::costs::FeeSchedule;
    using fin::strategy::Book;
    using fin::strategy::BookAssessment;
    using fin::strategy::BookPeriod;
    using fin::strategy::BookRow;
    using fin::strategy::CashflowKind;
    using fin::strategy::Equity;
    using fin::strategy::Portfolio;
    using fin::strategy::Projection;
    using fin::strategy::ProjectionPeriod;
    using fin::strategy::Reconciles;
    using fin::strategy::ReinvestmentPolicy;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;
    using fin::strategy::SpotPrice;
    using fin::strategy::WheelState;
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
            .currencyHandlingBasisPoints = 25 });
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

    RollRules NeverRolls()
    {
        return RollRules(RollTriggers{ .daysToExpiry = 0, .profitCaptureBasisPoints = 10000, .deltaLimit = Delta::FromValue(1.0) });
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

    Book Of(std::vector<std::string> underlyings, const Money& capital)
    {
        return Book(std::move(underlyings), Portfolio::Funded(capital), ReinvestmentPolicy::WithCashBuffer(0), NeverRolls(), Fees(), Rules());
    }

    std::vector<std::string> WrittenBy(std::span<const BookRow> rows)
    {
        std::vector<std::string> names;

        for (const auto& row : rows)
            if (row.row.contracts > 0)
                names.push_back(row.underlying);

        return names;
    }
}

TEST(TestBook, a_book_of_one_underlying_is_the_wheel_it_would_have_run_alone)
{
    const std::vector<ProjectionPeriod> periods{
        Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00),
        Period(On(2025, 7, 18), On(2026, 1, 16), 44.00, 0.90, 40.00),
        Period(On(2026, 1, 16), On(2026, 7, 17), 46.00, 1.10, 48.00)
    };

    Projection alone("ACME", Portfolio::Funded(Euro(10000.00)), ReinvestmentPolicy::WithCashBuffer(0), NeverRolls(), Fees(), Rules());
    auto book = Of({ "ACME" }, Euro(10000.00));

    for (const auto& period : periods)
    {
        const std::vector<BookPeriod> cycle{ { "ACME", period } };
        const auto expected = alone.Over(std::span<const ProjectionPeriod>(&period, 1));
        const auto rows = book.Over(cycle);

        ASSERT_EQ(rows.size(), 1u);
        EXPECT_EQ(rows.front().underlying, "ACME");
        EXPECT_EQ(rows.front().row, expected.front());
    }

    EXPECT_EQ(book.Holdings().Cash(), alone.Holdings().Cash());
    EXPECT_EQ(book.Ledger().Net(), alone.Ledger().Net());
}

TEST(TestBook, a_book_writes_one_row_for_every_underlying_in_the_order_it_was_listed)
{
    auto book = Of({ "ACME", "BETA" }, Euro(20000.00));

    const std::vector<BookPeriod> cycle{
        { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 50.00, 1.50, 55.00) },
        { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00) }
    };

    const auto rows = book.Over(cycle);

    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].underlying, "BETA");
    EXPECT_EQ(rows[1].underlying, "ACME");
}

TEST(TestBook, the_pot_is_shared_so_one_underlying_can_leave_nothing_for_another)
{
    auto book = Of({ "RICH", "POOR" }, Euro(5000.00));

    const std::vector<BookPeriod> cycle{
        { "RICH", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 2.00, 45.00) },
        { "POOR", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.10, 45.00) }
    };

    const auto rows = book.Over(cycle);

    EXPECT_EQ(WrittenBy(rows), (std::vector<std::string>{ "RICH" }));
    EXPECT_EQ(book.Holdings().Cash(), Euro(5199.25));
    EXPECT_TRUE(Reconciles(book.Holds()));
}

TEST(TestBook, the_richer_premium_takes_the_cash_whatever_order_the_book_lists_it_in)
{
    auto listedLast = Of({ "POOR", "RICH" }, Euro(5000.00));

    const std::vector<BookPeriod> cycle{
        { "POOR", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.10, 45.00) },
        { "RICH", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 2.00, 45.00) }
    };

    EXPECT_EQ(WrittenBy(listedLast.Over(cycle)), (std::vector<std::string>{ "RICH" }));
}

TEST(TestBook, a_wheel_holding_shares_writes_its_call_without_competing_for_cash)
{
    auto book = Of({ "ACME", "BETA" }, Euro(9000.00));

    const std::vector<BookPeriod> assigning{
        { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 35.00) },
        { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 50.00, 0.10, 55.00) }
    };

    book.Over(assigning);

    ASSERT_EQ(book.StateOf("ACME"), WheelState::HoldingShares);
    ASSERT_EQ(book.StateOf("BETA"), WheelState::Cash);

    const std::vector<BookPeriod> cycle{
        { "ACME", Period(On(2025, 7, 18), On(2026, 1, 16), 45.00, 1.00, 42.00) },
        { "BETA", Period(On(2025, 7, 18), On(2026, 1, 16), 50.00, 3.00, 55.00) }
    };

    const auto rows = book.Over(cycle);

    EXPECT_EQ(rows[0].row.contracts, 2);
    EXPECT_EQ(rows[1].row.contracts, 0);
}

TEST(TestBook, every_movement_of_cash_in_the_book_is_recorded_once)
{
    auto book = Of({ "ACME", "BETA" }, Euro(20000.00));

    const std::vector<BookPeriod> first{
        { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 35.00) },
        { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 50.00, 1.50, 55.00) }
    };

    const std::vector<BookPeriod> second{
        { "ACME", Period(On(2025, 7, 18), On(2026, 1, 16), 44.00, 0.90, 48.00) },
        { "BETA", Period(On(2025, 7, 18), On(2026, 1, 16), 52.00, 1.60, 50.00) }
    };

    book.Over(first);
    book.Over(second);

    EXPECT_TRUE(Reconciles(book.Holds()));
    EXPECT_TRUE(std::is_sorted(book.Ledger().Entries().begin(), book.Ledger().Entries().end(), [](const auto& left, const auto& right)
        {
            return left.on < right.on;
        }));
}

TEST(TestBook, two_underlyings_together_never_commit_more_than_one_wheel_would)
{
    auto book = Of({ "ONE", "TWO" }, Euro(16000.00));
    Projection alone("ONE", Portfolio::Funded(Euro(16000.00)), ReinvestmentPolicy::WithCashBuffer(0), NeverRolls(), Fees(), Rules());

    const auto shape = Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00);
    const std::vector<BookPeriod> cycle{ { "ONE", shape }, { "TWO", shape } };

    const auto rows = book.Over(cycle);
    const auto expected = alone.Over(std::span<const ProjectionPeriod>(&shape, 1));

    EXPECT_EQ(rows[0].row.contracts + rows[1].row.contracts, expected.front().contracts);
    EXPECT_EQ(rows[0].row.premium + rows[1].row.premium, expected.front().premium);
    EXPECT_EQ(book.Holdings().Cash(), alone.Holdings().Cash());
}

TEST(TestBook, a_book_refuses_a_period_for_an_underlying_it_does_not_carry)
{
    auto book = Of({ "ACME" }, Euro(20000.00));
    const std::vector<BookPeriod> stranger{ { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00) } };

    EXPECT_DEATH(book.Over(stranger), "");
}

TEST(TestBook, a_book_refuses_the_same_underlying_twice_in_one_cycle)
{
    auto book = Of({ "ACME", "BETA" }, Euro(20000.00));
    const auto shape = Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00);
    const std::vector<BookPeriod> twice{ { "ACME", shape }, { "ACME", shape } };

    EXPECT_DEATH(book.Over(twice), "");
}

TEST(TestBook, a_cycle_covers_every_underlying_the_book_carries)
{
    auto book = Of({ "ACME", "BETA" }, Euro(20000.00));
    const std::vector<BookPeriod> onlyOne{ { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00) } };

    EXPECT_DEATH(book.Over(onlyOne), "");
}

TEST(TestBook, a_hand_checked_book_of_two_underlyings_over_two_cycles)
{
    auto book = Of({ "ACME", "BETA" }, Euro(20000.00));

    const std::vector<BookPeriod> first{
        { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 1.20, 35.00) },
        { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 50.00, 1.00, 55.00) }
    };

    const auto opening = book.Over(first);

    EXPECT_EQ(opening[0].row.contracts, 5);
    EXPECT_EQ(opening[0].row.premium, Euro(600.00));
    EXPECT_EQ(opening[0].row.fees, Euro(6.75));
    EXPECT_EQ(opening[0].row.assignment, Euro(-20000.00));
    EXPECT_EQ(opening[0].row.shares, 500);
    EXPECT_EQ(opening[1].row.contracts, 0);
    EXPECT_EQ(book.Holdings().Cash(), Euro(593.25));

    const std::vector<BookPeriod> second{
        { "ACME", Period(On(2025, 7, 18), On(2026, 1, 16), 38.00, 0.90, 42.00) },
        { "BETA", Period(On(2025, 7, 18), On(2026, 1, 16), 50.00, 1.00, 52.00) }
    };

    const auto closing = book.Over(second);

    EXPECT_EQ(closing[0].row.contracts, 5);
    EXPECT_EQ(closing[0].row.premium, Euro(450.00));
    EXPECT_EQ(closing[0].row.assignment, Euro(19000.00));
    EXPECT_EQ(closing[0].row.shares, 0);
    EXPECT_EQ(closing[1].row.contracts, 0);
    EXPECT_EQ(book.Holdings().Cash(), Euro(20036.50));
    EXPECT_EQ(closing[0].row.equity, Euro(20036.50));
    EXPECT_TRUE(Reconciles(book.Holds()));
}

TEST(TestBook, a_wheel_inside_a_book_never_strikes_the_tax_itself)
{
    auto book = Of({ "ACME" }, Euro(200000.00));
    auto period = Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00);
    period.assessed = fin::strategy::TaxObservation{ On(2025, 1, 6), Euro(41.00) };

    const std::vector<BookPeriod> cycle{ { "ACME", period } };

    EXPECT_DEATH(book.Over(cycle), "");
}

TEST(TestBook, the_charge_falls_on_the_book_as_a_whole)
{
    auto book = Of({ "ACME", "BETA" }, Euro(200000.00));
    Book separately = Of({ "ACME" }, Euro(200000.00));

    const std::vector<SpotPrice> prices{ { "ACME", Euro(45.00) }, { "BETA", Euro(50.00) } };
    const std::vector<SpotPrice> together{ { "ACME", Euro(45.00) } };

    EXPECT_EQ(book.Assess({ On(2026, 1, 1), prices }), separately.Assess({ On(2026, 1, 1), together }));
}

TEST(TestBook, the_allowance_is_granted_to_the_book_and_not_to_every_underlying_in_it)
{
    auto book = Of({ "ACME", "BETA" }, Euro(60000.00));
    auto half = Of({ "ACME" }, Euro(30000.00));

    const std::vector<SpotPrice> whole{ { "ACME", Euro(45.00) }, { "BETA", Euro(50.00) } };
    const std::vector<SpotPrice> part{ { "ACME", Euro(45.00) } };

    const auto charged = book.Assess({ On(2026, 1, 1), whole });

    EXPECT_GT(charged.Cents(), 0);
    EXPECT_EQ(half.Assess({ On(2026, 1, 1), part }), Euro(0.00));
}

TEST(TestBook, a_book_below_the_allowance_is_charged_nothing_however_much_it_earned)
{
    auto book = Of({ "ACME", "BETA" }, Euro(20000.00));

    const std::vector<BookPeriod> cycle{
        { "ACME", Period(On(2025, 1, 3), On(2025, 7, 18), 40.00, 0.80, 45.00) },
        { "BETA", Period(On(2025, 1, 3), On(2025, 7, 18), 50.00, 1.50, 55.00) }
    };

    book.Over(cycle);

    const std::vector<SpotPrice> prices{ { "ACME", Euro(45.00) }, { "BETA", Euro(55.00) } };

    EXPECT_EQ(book.Assess({ On(2026, 1, 1), prices }), Euro(0.00));
    EXPECT_TRUE(book.Ledger().Entries().end() == std::find_if(book.Ledger().Entries().begin(), book.Ledger().Entries().end(), [](const auto& entry)
                                                     {
                                                         return entry.kind == CashflowKind::Tax;
                                                     }));
}

TEST(TestBook, a_charge_is_paid_out_of_the_book_and_recorded_on_the_day_it_was_struck)
{
    auto book = Of({ "ACME", "BETA" }, Euro(200000.00));

    const std::vector<SpotPrice> prices{ { "ACME", Euro(45.00) }, { "BETA", Euro(50.00) } };
    const auto before = book.Holdings().Cash();
    const auto charged = book.Assess({ On(2026, 1, 1), prices });

    EXPECT_GT(charged.Cents(), 0);
    EXPECT_EQ(book.Holdings().Cash(), before - charged);
    EXPECT_EQ(book.Ledger().NetOf(CashflowKind::Tax), -charged);
    EXPECT_EQ(book.Ledger().Entries().front().on, On(2026, 1, 1));
    EXPECT_TRUE(Reconciles(book.Holds()));
}

TEST(TestBook, a_book_cannot_be_listed_empty_or_with_the_same_underlying_twice)
{
    EXPECT_DEATH(Of({}, Euro(20000.00)), "");
    EXPECT_DEATH(Of({ "ACME", "ACME" }, Euro(20000.00)), "");
}
