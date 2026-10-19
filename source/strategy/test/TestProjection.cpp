#include "source/costs/BrokerFees.hpp"
#include "source/strategy/Projection.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <optional>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Delta;
    using fin::core::Money;
    using fin::core::OptionContract;
    using fin::core::OptionType;
    using fin::costs::BrokerFees;
    using fin::costs::FeeSchedule;
    using fin::strategy::Account;
    using fin::strategy::CashflowKind;
    using fin::strategy::Portfolio;
    using fin::strategy::Projection;
    using fin::strategy::ProjectionPeriod;
    using fin::strategy::ProjectionRow;
    using fin::strategy::Reconciles;
    using fin::strategy::ReinvestmentPolicy;
    using fin::strategy::RollObservation;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;
    using fin::strategy::TaxObservation;
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

    RollRules NeverRolls()
    {
        return RollRules(RollTriggers{ .daysToExpiry = 0, .profitCaptureBasisPoints = 10000, .deltaLimit = Delta::FromValue(1.0) });
    }

    RollRules RollsOnHalfTheProfit()
    {
        return RollRules(RollTriggers{ .daysToExpiry = 7, .profitCaptureBasisPoints = 5000, .deltaLimit = Delta::FromValue(0.45) });
    }

    Projection Wheel(const Money& capital, const ReinvestmentPolicy& policy, const RollRules& rules)
    {
        return Projection("ACME", Portfolio::Funded(capital), policy, rules, Fees(), Rules());
    }

    Projection WheelOver(Account& account, const ReinvestmentPolicy& policy, const RollRules& rules)
    {
        return Projection("ACME", account, policy, rules, Fees());
    }

    class TestProjection
        : public ::testing::Test
    {
    protected:
        ReinvestmentPolicy fullyInvested{ ReinvestmentPolicy::WithCashBuffer(0) };
    };

    class TestProjectionOverThreeYears
        : public ::testing::Test
    {
    protected:
        TestProjectionOverThreeYears()
        {
            rows = wheel.Over(periods);
        }

        std::vector<ProjectionPeriod> periods{
            { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(40.00), .premiumPerShare = 0.80, .spotAtExpiry = Euro(45.00), .observed = std::nullopt, .assessed = std::nullopt },
            { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(44.00), .premiumPerShare = 0.90, .spotAtExpiry = Euro(40.00), .observed = std::nullopt, .assessed = std::nullopt },
            { .opened = On(2026, 1, 16), .expiry = On(2026, 7, 17), .strike = Euro(46.00), .premiumPerShare = 1.10, .spotAtExpiry = Euro(48.00), .observed = std::nullopt, .assessed = std::nullopt },
            { .opened = On(2026, 7, 17), .expiry = On(2027, 1, 15), .strike = Euro(48.00), .premiumPerShare = 1.20, .spotAtExpiry = Euro(52.00), .observed = std::nullopt, .assessed = std::nullopt },
            { .opened = On(2027, 1, 15), .expiry = On(2027, 7, 16), .strike = Euro(52.00), .premiumPerShare = 1.30, .spotAtExpiry = Euro(50.00), .observed = std::nullopt, .assessed = std::nullopt },
            { .opened = On(2027, 7, 16), .expiry = On(2028, 1, 14), .strike = Euro(54.00), .premiumPerShare = 1.40, .spotAtExpiry = Euro(53.00), .observed = std::nullopt, .assessed = std::nullopt }
        };

        Money capital{ Euro(10000.00) };
        Projection wheel{ Wheel(capital, ReinvestmentPolicy::WithCashBuffer(0), NeverRolls()) };
        std::vector<ProjectionRow> rows;
    };
}

TEST_F(TestProjectionOverThreeYears, one_row_is_emitted_for_every_period)
{
    ASSERT_EQ(rows.size(), periods.size());

    for (std::size_t period = 0; period != periods.size(); ++period)
        EXPECT_EQ(rows[period].on, periods[period].expiry);
}

TEST_F(TestProjectionOverThreeYears, the_first_put_expires_worthless_and_the_premium_is_kept)
{
    EXPECT_EQ(rows[0].state, WheelState::Cash);
    EXPECT_EQ(rows[0].contracts, 2);
    EXPECT_EQ(rows[0].premium, Euro(160.00));
    EXPECT_EQ(rows[0].fees, Euro(1.50));
    EXPECT_EQ(rows[0].assignment, Euro(0.00));
    EXPECT_EQ(rows[0].cash, Euro(10158.50));
    EXPECT_EQ(rows[0].shares, 0);
    EXPECT_EQ(rows[0].equity, Euro(10158.50));
}

TEST_F(TestProjectionOverThreeYears, the_second_put_is_assigned_and_the_reserved_cash_becomes_shares)
{
    EXPECT_EQ(rows[1].state, WheelState::HoldingShares);
    EXPECT_EQ(rows[1].contracts, 2);
    EXPECT_EQ(rows[1].premium, Euro(180.00));
    EXPECT_EQ(rows[1].fees, Euro(4.50));
    EXPECT_EQ(rows[1].assignment, Euro(-8800.00));
    EXPECT_EQ(rows[1].cash, Euro(1534.00));
    EXPECT_EQ(rows[1].shares, 200);
    EXPECT_EQ(rows[1].equity, Euro(9534.00));
}

TEST_F(TestProjectionOverThreeYears, the_covered_call_is_assigned_and_the_shares_become_cash_again)
{
    EXPECT_EQ(rows[2].state, WheelState::Cash);
    EXPECT_EQ(rows[2].contracts, 2);
    EXPECT_EQ(rows[2].premium, Euro(220.00));
    EXPECT_EQ(rows[2].fees, Euro(4.50));
    EXPECT_EQ(rows[2].assignment, Euro(9200.00));
    EXPECT_EQ(rows[2].cash, Euro(10949.50));
    EXPECT_EQ(rows[2].shares, 0);
    EXPECT_EQ(rows[2].equity, Euro(10949.50));
}

TEST_F(TestProjectionOverThreeYears, the_grown_balance_still_only_secures_two_contracts_at_a_higher_strike)
{
    EXPECT_EQ(rows[3].state, WheelState::Cash);
    EXPECT_EQ(rows[3].contracts, 2);
    EXPECT_EQ(rows[3].premium, Euro(240.00));
    EXPECT_EQ(rows[3].fees, Euro(1.50));
    EXPECT_EQ(rows[3].cash, Euro(11188.00));
    EXPECT_EQ(rows[3].equity, Euro(11188.00));
}

TEST_F(TestProjectionOverThreeYears, the_second_assignment_leaves_the_portfolio_holding_shares_below_their_strike)
{
    EXPECT_EQ(rows[4].state, WheelState::HoldingShares);
    EXPECT_EQ(rows[4].premium, Euro(260.00));
    EXPECT_EQ(rows[4].fees, Euro(4.50));
    EXPECT_EQ(rows[4].assignment, Euro(-10400.00));
    EXPECT_EQ(rows[4].cash, Euro(1043.50));
    EXPECT_EQ(rows[4].shares, 200);
    EXPECT_EQ(rows[4].equity, Euro(11043.50));
}

TEST_F(TestProjectionOverThreeYears, a_call_that_expires_out_of_the_money_leaves_the_shares_where_they_are)
{
    EXPECT_EQ(rows[5].state, WheelState::HoldingShares);
    EXPECT_EQ(rows[5].contracts, 2);
    EXPECT_EQ(rows[5].premium, Euro(280.00));
    EXPECT_EQ(rows[5].fees, Euro(1.50));
    EXPECT_EQ(rows[5].assignment, Euro(0.00));
    EXPECT_EQ(rows[5].cash, Euro(1322.00));
    EXPECT_EQ(rows[5].shares, 200);
    EXPECT_EQ(rows[5].equity, Euro(11922.00));
}

TEST_F(TestProjectionOverThreeYears, the_ledger_accounts_for_every_movement_of_cash)
{
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Premium), Euro(1340.00));
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Fee), Euro(-18.00));
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Assignment), Euro(-10000.00));
    EXPECT_EQ(capital + wheel.Ledger().Net(), wheel.Holdings().Cash());
}

TEST_F(TestProjectionOverThreeYears, the_projection_ends_where_the_last_row_says_it_does)
{
    EXPECT_EQ(wheel.State(), rows.back().state);
    EXPECT_EQ(wheel.Holdings().Cash(), rows.back().cash);
    EXPECT_EQ(wheel.Holdings().Shares("ACME"), rows.back().shares);
}

TEST_F(TestProjectionOverThreeYears, a_wheel_run_over_an_account_it_does_not_own_produces_the_same_table)
{
    Account account(Portfolio::Funded(capital), Rules());
    auto lodger = WheelOver(account, ReinvestmentPolicy::WithCashBuffer(0), NeverRolls());

    EXPECT_EQ(lodger.Over(periods), rows);
    EXPECT_EQ(account.Holdings().Cash(), wheel.Holdings().Cash());
    EXPECT_EQ(account.Ledger().Net(), wheel.Ledger().Net());
    EXPECT_TRUE(Reconciles(account));
}

TEST_F(TestProjectionOverThreeYears, a_wheel_reports_the_account_it_was_given_rather_than_one_of_its_own)
{
    Account account(Portfolio::Funded(capital), Rules());
    auto lodger = WheelOver(account, ReinvestmentPolicy::WithCashBuffer(0), NeverRolls());

    lodger.Over(periods);

    EXPECT_EQ(&lodger.Holdings(), &account.Holdings());
    EXPECT_EQ(&lodger.Ledger(), &account.Ledger());
}

TEST_F(TestProjectionOverThreeYears, a_copy_of_a_wheel_that_owns_its_account_leaves_the_original_alone)
{
    auto original = Wheel(capital, ReinvestmentPolicy::WithCashBuffer(0), NeverRolls());
    auto copy = original;

    copy.Over(periods);

    EXPECT_EQ(original.Holdings().Cash(), capital);
    EXPECT_TRUE(original.Ledger().Entries().empty());
    EXPECT_EQ(copy.Holdings().Cash(), rows.back().cash);
}

TEST_F(TestProjectionOverThreeYears, a_copy_of_a_wheel_that_was_lent_an_account_is_still_lent_the_same_one)
{
    Account account(Portfolio::Funded(capital), Rules());
    auto lodger = WheelOver(account, ReinvestmentPolicy::WithCashBuffer(0), NeverRolls());
    auto copy = lodger;

    copy.Over(periods);

    EXPECT_EQ(account.Holdings().Cash(), rows.back().cash);
    EXPECT_EQ(&copy.Holdings(), &account.Holdings());
}

TEST_F(TestProjectionOverThreeYears, a_projection_with_no_reference_date_in_it_is_charged_no_tax)
{
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Tax), Euro(0.00));

    for (const auto& row : rows)
        EXPECT_EQ(row.tax, Euro(0.00));
}

TEST_F(TestProjection, a_period_that_cannot_secure_a_single_contract_does_nothing)
{
    auto wheel = Wheel(Euro(1000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(40.00), .premiumPerShare = 0.80, .spotAtExpiry = Euro(35.00), .observed = std::nullopt, .assessed = std::nullopt }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].contracts, 0);
    EXPECT_EQ(rows[0].premium, Euro(0.00));
    EXPECT_EQ(rows[0].fees, Euro(0.00));
    EXPECT_EQ(rows[0].cash, Euro(1000.00));
    EXPECT_EQ(rows[0].state, WheelState::Cash);
    EXPECT_TRUE(wheel.Ledger().Entries().empty());
}

TEST_F(TestProjection, the_cash_buffer_costs_the_contract_it_would_have_paid_for)
{
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(45.00), .premiumPerShare = 0.80, .spotAtExpiry = Euro(50.00), .observed = std::nullopt, .assessed = std::nullopt }
    };

    auto committed = Wheel(Euro(10000.00), fullyInvested, NeverRolls());
    auto cautious = Wheel(Euro(10000.00), ReinvestmentPolicy::WithCashBuffer(1500), NeverRolls());

    EXPECT_EQ(committed.Over(periods)[0].contracts, 2);
    EXPECT_EQ(cautious.Over(periods)[0].contracts, 1);
}

TEST_F(TestProjection, a_position_that_was_rolled_away_cannot_be_assigned)
{
    auto wheel = Wheel(Euro(10000.00), fullyInvested, RollsOnHalfTheProfit());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(40.00), .premiumPerShare = 0.80, .spotAtExpiry = Euro(35.00), .observed = RollObservation{ .on = On(2025, 6, 18), .valuePerShare = 0.20, .delta = Delta::FromValue(-0.20) }, .assessed = std::nullopt }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].state, WheelState::Cash);
    EXPECT_EQ(rows[0].contracts, 2);
    EXPECT_EQ(rows[0].premium, Euro(120.00));
    EXPECT_EQ(rows[0].fees, Euro(3.00));
    EXPECT_EQ(rows[0].assignment, Euro(0.00));
    EXPECT_EQ(rows[0].shares, 0);
    EXPECT_EQ(rows[0].cash, Euro(10117.00));
}

TEST_F(TestProjection, a_position_the_rules_leave_alone_runs_to_expiry_and_is_assigned)
{
    auto wheel = Wheel(Euro(10000.00), fullyInvested, RollsOnHalfTheProfit());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(40.00), .premiumPerShare = 0.80, .spotAtExpiry = Euro(35.00), .observed = RollObservation{ .on = On(2025, 6, 18), .valuePerShare = 0.70, .delta = Delta::FromValue(-0.20) }, .assessed = std::nullopt }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].state, WheelState::HoldingShares);
    EXPECT_EQ(rows[0].shares, 200);
    EXPECT_EQ(rows[0].assignment, Euro(-8000.00));
}

TEST_F(TestProjection, a_portfolio_that_already_holds_shares_starts_by_writing_calls_against_them)
{
    auto holder = Portfolio::Funded(Euro(1000.00));
    holder.SellPut(OptionContract("ACME", On(2024, 12, 20), Euro(9.00), OptionType::Put), 1);
    holder.Assign(OptionContract("ACME", On(2024, 12, 20), Euro(9.00), OptionType::Put));

    Projection wheel("ACME", holder, fullyInvested, NeverRolls(), Fees(), Rules());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(12.00), .premiumPerShare = 0.50, .spotAtExpiry = Euro(11.00), .observed = std::nullopt, .assessed = std::nullopt }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].contracts, 1);
    EXPECT_EQ(rows[0].premium, Euro(50.00));
    EXPECT_EQ(rows[0].state, WheelState::HoldingShares);
    EXPECT_EQ(rows[0].shares, 100);
}

TEST_F(TestProjection, a_projection_needs_an_underlying_and_a_portfolio_with_nothing_open)
{
    auto open = Portfolio::Funded(Euro(10000.00));
    open.SellPut(OptionContract("ACME", On(2025, 7, 18), Euro(40.00), OptionType::Put), 1);

    EXPECT_DEATH(Projection("", Portfolio::Funded(Euro(10000.00)), fullyInvested, NeverRolls(), Fees(), Rules()), "");
    EXPECT_DEATH(Projection("ACME", open, fullyInvested, NeverRolls(), Fees(), Rules()), "");
}

TEST_F(TestProjection, a_wheel_is_only_refused_an_account_that_is_already_short_of_its_own_underlying)
{
    Account account(Portfolio::Funded(Euro(10000.00)), Rules());
    account.Holdings().SellPut(OptionContract("BETA", On(2025, 7, 18), Euro(40.00), OptionType::Put), 1);

    const Projection acme("ACME", account, fullyInvested, NeverRolls(), Fees());

    EXPECT_EQ(acme.State(), WheelState::Cash);
    EXPECT_DEATH(Projection("BETA", account, fullyInvested, NeverRolls(), Fees()), "");
}

TEST_F(TestProjection, the_annual_charge_falls_on_what_is_held_on_the_reference_date)
{
    auto wheel = Wheel(Euro(200000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(40.00), .premiumPerShare = 1.00, .spotAtExpiry = Euro(45.00), .observed = std::nullopt, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(50.00) } }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].contracts, 50);
    EXPECT_EQ(rows[0].premium, Euro(5000.00));
    EXPECT_EQ(rows[0].fees, Euro(37.50));
    EXPECT_EQ(rows[0].tax, Euro(726.38));
    EXPECT_EQ(rows[0].cash, Euro(204236.12));
    EXPECT_EQ(rows[0].equity, Euro(204236.12));
}

TEST_F(TestProjection, the_annual_charge_is_dated_on_the_reference_date_and_not_on_the_expiry)
{
    auto wheel = Wheel(Euro(200000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(40.00), .premiumPerShare = 1.00, .spotAtExpiry = Euro(45.00), .observed = std::nullopt, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(50.00) } }
    };

    wheel.Over(periods);
    const auto entries = wheel.Ledger().Entries();
    const auto charged = std::find_if(entries.begin(), entries.end(), [](const auto& entry)
        {
            return entry.kind == CashflowKind::Tax;
        });

    ASSERT_NE(charged, entries.end());
    EXPECT_EQ(charged->on, On(2026, 1, 1));
    EXPECT_EQ(charged->amount, Euro(-726.38));
}

TEST_F(TestProjection, the_ledger_still_reconciles_once_the_tax_has_been_struck)
{
    auto wheel = Wheel(Euro(200000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(40.00), .premiumPerShare = 1.00, .spotAtExpiry = Euro(45.00), .observed = std::nullopt, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(50.00) } }
    };

    wheel.Over(periods);

    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Tax), Euro(-726.38));
    EXPECT_EQ(Euro(200000.00) + wheel.Ledger().Net(), wheel.Holdings().Cash());
}

TEST_F(TestProjection, a_portfolio_below_the_allowance_pays_no_tax_however_much_premium_it_earned)
{
    auto wheel = Wheel(Euro(40000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(40.00), .premiumPerShare = 5.00, .spotAtExpiry = Euro(45.00), .observed = std::nullopt, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(50.00) } }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].premium, Euro(5000.00));
    EXPECT_EQ(rows[0].tax, Euro(0.00));
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Tax), Euro(0.00));
}

TEST_F(TestProjection, a_reference_date_the_projection_reaches_without_a_position_is_still_struck)
{
    auto wheel = Wheel(Euro(200000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(400000.00), .premiumPerShare = 1.00, .spotAtExpiry = Euro(450000.00), .observed = std::nullopt, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(500000.00) } }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].contracts, 0);
    EXPECT_EQ(rows[0].premium, Euro(0.00));
    EXPECT_GT(rows[0].tax, Euro(0.00));
}

TEST_F(TestProjection, a_position_closed_before_the_reference_date_is_not_part_of_what_is_taxed)
{
    const std::vector<ProjectionPeriod> rolled{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(40.00), .premiumPerShare = 1.00, .spotAtExpiry = Euro(35.00), .observed = RollObservation{ .on = On(2025, 12, 15), .valuePerShare = 0.20, .delta = Delta::FromValue(-0.20) }, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(30.00) } }
    };
    const std::vector<ProjectionPeriod> held{
        { .opened = On(2025, 7, 18), .expiry = On(2026, 1, 16), .strike = Euro(40.00), .premiumPerShare = 1.00, .spotAtExpiry = Euro(35.00), .observed = RollObservation{ .on = On(2025, 12, 15), .valuePerShare = 0.70, .delta = Delta::FromValue(-0.20) }, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(30.00) } }
    };

    auto closes = Wheel(Euro(200000.00), fullyInvested, RollsOnHalfTheProfit());
    auto keeps = Wheel(Euro(200000.00), fullyInvested, RollsOnHalfTheProfit());

    EXPECT_GT(closes.Over(rolled)[0].tax, keeps.Over(held)[0].tax);
}

TEST_F(TestProjection, shares_held_through_a_cycle_earn_the_dividend_net_of_withholding)
{
    auto holder = Portfolio::Funded(Euro(1000.00));
    holder.SellPut(OptionContract("ACME", On(2024, 12, 20), Euro(9.00), OptionType::Put), 1);
    holder.Assign(OptionContract("ACME", On(2024, 12, 20), Euro(9.00), OptionType::Put));

    Projection wheel("ACME", holder, fullyInvested, NeverRolls(), Fees(), Rules());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(12.00), .premiumPerShare = 0.50, .spotAtExpiry = Euro(11.00), .observed = std::nullopt, .assessed = std::nullopt, .dividendPerShare = 0.30, .dividendWithholdingBasisPoints = 1500 }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].dividend, Euro(25.50));
    EXPECT_EQ(rows[0].cash, Euro(174.75));
    EXPECT_EQ(rows[0].shares, 100);
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Dividend), Euro(25.50));
    EXPECT_EQ(wheel.Ledger().Entries().back().on, On(2025, 7, 18));
    EXPECT_EQ(Euro(100.00) + wheel.Ledger().Net(), wheel.Holdings().Cash());
}

TEST_F(TestProjection, a_cycle_spent_in_cash_earns_no_dividend_even_when_it_ends_in_shares)
{
    auto wheel = Wheel(Euro(10000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 1, 3), .expiry = On(2025, 7, 18), .strike = Euro(40.00), .premiumPerShare = 0.80, .spotAtExpiry = Euro(35.00), .observed = std::nullopt, .assessed = std::nullopt, .dividendPerShare = 1.00, .dividendWithholdingBasisPoints = 1500 }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].dividend, Euro(0.00));
    EXPECT_EQ(rows[0].shares, 200);
    EXPECT_EQ(wheel.Ledger().NetOf(CashflowKind::Dividend), Euro(0.00));
}

TEST_F(TestProjection, a_charge_the_free_cash_cannot_meet_is_borrowed_and_repaid_from_the_next_premium)
{
    auto wheel = Wheel(Euro(200000.00), fullyInvested, NeverRolls());
    const std::vector<ProjectionPeriod> periods{
        { .opened = On(2025, 12, 1), .expiry = On(2025, 12, 31), .strike = Euro(40.00), .premiumPerShare = 0.50, .spotAtExpiry = Euro(35.00), .observed = std::nullopt, .assessed = std::nullopt },
        { .opened = On(2025, 12, 31), .expiry = On(2026, 1, 30), .strike = Euro(38.00), .premiumPerShare = 0.01, .spotAtExpiry = Euro(36.00), .observed = std::nullopt, .assessed = TaxObservation{ .on = On(2026, 1, 1), .spot = Euro(35.00) } },
        { .opened = On(2026, 1, 30), .expiry = On(2026, 3, 1), .strike = Euro(40.00), .premiumPerShare = 0.60, .spotAtExpiry = Euro(36.00), .observed = std::nullopt, .assessed = std::nullopt }
    };

    const auto rows = wheel.Over(periods);

    EXPECT_EQ(rows[0].shares, 5000);
    EXPECT_GT(rows[1].tax, rows[1].cash + rows[1].tax);
    EXPECT_LT(rows[1].cash, Euro(0.00));
    EXPECT_TRUE(wheel.Holdings().Overdrawn() == false);
    EXPECT_GT(rows[2].cash, Euro(0.00));
    EXPECT_LT(wheel.Ledger().NetOf(CashflowKind::Interest), Euro(0.00));
    EXPECT_EQ(Euro(200000.00) + wheel.Ledger().Net(), wheel.Holdings().Cash());
}
