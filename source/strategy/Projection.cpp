#include "source/strategy/Projection.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/core/OptionContract.hpp"
#include "source/costs/Trade.hpp"
#include "source/pricing/AssignmentModel.hpp"
#include <algorithm>
#include <utility>

namespace fin::strategy
{
    namespace
    {
        struct Movements
        {
            std::int64_t contracts;
            core::Money premium;
            core::Money fees;
            core::Money assignment;
            core::Money tax;
        };

        Movements Nothing(core::Currency currency)
        {
            return { 0, core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency) };
        }

        core::Money PerContract(double perShare, const core::OptionContract& contract, core::Currency currency)
        {
            return core::Money::FromAmount(perShare, currency) * static_cast<std::int64_t>(contract.Multiplier());
        }

        std::int64_t SharesUnder(const core::OptionContract& contract, std::int64_t contracts)
        {
            return static_cast<std::int64_t>(contract.Multiplier()) * contracts;
        }

        tax::Box3Assets AssetsOf(const Portfolio& portfolio, const core::Money& spot)
        {
            const auto equity = Equity(portfolio, spot);
            const auto deposits = std::min(portfolio.Cash(), equity);

            return { deposits, equity - deposits };
        }

        ProjectionRow RowFor(const ProjectionPeriod& period, WheelState state, const Movements& movements, const Portfolio& portfolio)
        {
            return { period.expiry, state, movements.contracts, movements.premium, movements.fees, movements.assignment, movements.tax, portfolio.Cash(), portfolio.Shares(), Equity(portfolio, period.spotAtExpiry) };
        }
    }

    Projection::Projection(std::string underlying, const Portfolio& start, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees, const tax::Box3TaxModel& taxes)
        : underlying(std::move(underlying))
        , portfolio(start)
        , policy(policy)
        , rules(rules)
        , brokerFees(brokerFees)
        , taxes(taxes)
        , ledger(start.GetCurrency())
        , state(start.Shares() > 0 ? WheelState::HoldingShares : WheelState::Cash)
    {
        really_assert(!this->underlying.empty());
        really_assert(start.ShortPositions().empty());
    }

    std::vector<ProjectionRow> Projection::Over(std::span<const ProjectionPeriod> periods)
    {
        std::vector<ProjectionRow> rows;
        rows.reserve(periods.size());

        for (const auto& period : periods)
            rows.push_back(Advance(period));

        return rows;
    }

    WheelState Projection::State() const
    {
        return state;
    }

    const Portfolio& Projection::Holdings() const
    {
        return portfolio;
    }

    const CompoundingLedger& Projection::Ledger() const
    {
        return ledger;
    }

    ProjectionRow Projection::Advance(const ProjectionPeriod& period)
    {
        const auto currency = portfolio.GetCurrency();

        really_assert(period.opened <= period.expiry);
        really_assert(period.premiumPerShare >= 0.0);
        really_assert(period.strike.GetCurrency() == currency);
        really_assert(period.spotAtExpiry.GetCurrency() == currency);
        really_assert(state == WheelState::Cash || state == WheelState::HoldingShares);

        const auto writingPut = state == WheelState::Cash;
        const core::OptionContract contract(underlying, period.expiry, period.strike, writingPut ? core::OptionType::Put : core::OptionType::Call);

        auto movements = Nothing(currency);
        auto assessment = period.assessed;

        const auto assess = [&](const core::Date& upTo)
        {
            if (!assessment.has_value() || assessment->on > upTo)
                return;

            really_assert(assessment->on >= period.opened && assessment->on <= period.expiry);
            really_assert(assessment->spot.GetCurrency() == currency);

            const auto on = assessment->on;
            const auto charge = taxes.Charge(AssetsOf(portfolio, assessment->spot));
            assessment.reset();

            if (charge.Cents() == 0)
                return;

            portfolio.Pay(charge);
            ledger.Post(on, CashflowKind::Tax, -charge);
            movements.tax += charge;
        };

        movements.contracts = writingPut
                                  ? policy.ContractsFor(portfolio.FreeCash(), contract.Notional())
                                  : portfolio.UncoveredShares() / static_cast<std::int64_t>(contract.Multiplier());

        if (movements.contracts == 0)
        {
            assess(period.expiry);
            return RowFor(period, state, movements, portfolio);
        }

        if (writingPut)
            portfolio.SellPut(contract, movements.contracts);
        else
            portfolio.SellCall(contract, movements.contracts);

        MoveOn(writingPut ? WheelEvent::SellPut : WheelEvent::SellCall);

        const auto received = PerContract(period.premiumPerShare, contract, currency) * movements.contracts;
        portfolio.Receive(received);
        ledger.Post(period.opened, CashflowKind::Premium, received);
        movements.premium += received;

        const auto openingFee = brokerFees.For(costs::Trade::Options(movements.contracts));
        portfolio.Pay(openingFee);
        ledger.Post(period.opened, CashflowKind::Fee, -openingFee);
        movements.fees += openingFee;

        if (Rolls(period))
        {
            assess(period.observed->on);

            const auto paid = PerContract(period.observed->valuePerShare, contract, currency) * movements.contracts;
            portfolio.Close(contract);
            portfolio.Pay(paid);
            ledger.Post(period.observed->on, CashflowKind::Premium, -paid);
            movements.premium -= paid;

            const auto closingFee = brokerFees.For(costs::Trade::Options(movements.contracts));
            portfolio.Pay(closingFee);
            ledger.Post(period.observed->on, CashflowKind::Fee, -closingFee);
            movements.fees += closingFee;

            MoveOn(WheelEvent::Closed);
            assess(period.expiry);
            return RowFor(period, state, movements, portfolio);
        }

        assess(period.expiry);

        if (pricing::AssignmentModel::AtExpiry(contract, period.spotAtExpiry) == pricing::AssignmentOutcome::NotAssigned)
        {
            portfolio.Close(contract);
            MoveOn(WheelEvent::ExpiredWorthless);
            return RowFor(period, state, movements, portfolio);
        }

        const auto settlement = contract.Notional() * movements.contracts;
        portfolio.Assign(contract);
        movements.assignment = writingPut ? -settlement : settlement;
        ledger.Post(period.expiry, CashflowKind::Assignment, movements.assignment);

        const auto settlementFee = brokerFees.For(costs::Trade::Shares(SharesUnder(contract, movements.contracts)));
        portfolio.Pay(settlementFee);
        ledger.Post(period.expiry, CashflowKind::Fee, -settlementFee);
        movements.fees += settlementFee;

        MoveOn(WheelEvent::Assigned);
        return RowFor(period, state, movements, portfolio);
    }

    bool Projection::Rolls(const ProjectionPeriod& period) const
    {
        if (!period.observed.has_value())
            return false;

        really_assert(period.observed->on >= period.opened && period.observed->on <= period.expiry);
        really_assert(period.premiumPerShare > 0.0);

        return rules
            .Triggered({ .daysToExpiry = period.expiry - period.observed->on,
                .premiumReceivedPerShare = period.premiumPerShare,
                .valuePerShare = period.observed->valuePerShare,
                .delta = period.observed->delta })
            .has_value();
    }

    void Projection::MoveOn(WheelEvent event)
    {
        const auto next = WheelEngine::Next(state, event);
        really_assert(next.has_value());

        state = *next;
    }
}
