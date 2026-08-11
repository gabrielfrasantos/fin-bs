#include "source/strategy/Projection.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/core/OptionContract.hpp"
#include "source/costs/Trade.hpp"
#include "source/pricing/AssignmentModel.hpp"
#include <algorithm>
#include <array>
#include <utility>

namespace fin::strategy
{
    namespace
    {
        core::Money PerContract(double perShare, const core::OptionContract& contract, core::Currency currency)
        {
            return core::Money::FromAmount(perShare, currency) * static_cast<std::int64_t>(contract.Multiplier());
        }

        std::int64_t SharesUnder(const core::OptionContract& contract, std::int64_t contracts)
        {
            return static_cast<std::int64_t>(contract.Multiplier()) * contracts;
        }

        bool WritesNothingYet(const Portfolio& portfolio, const std::string& underlying)
        {
            const auto positions = portfolio.ShortPositions();

            return std::none_of(positions.begin(), positions.end(), [&underlying](const ShortOption& open)
                {
                    return open.contract.Underlying() == underlying;
                });
        }

        tax::Box3Assets AssetsOf(const Portfolio& portfolio, const std::string& underlying, const core::Money& spot)
        {
            const auto equity = Equity(portfolio, underlying, spot);
            const auto deposits = std::min(portfolio.Cash(), equity);

            return { deposits, equity - deposits };
        }
    }

    Projection::Projection(std::string underlying, const Portfolio& start, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees, const tax::Box3TaxModel& taxes)
        : underlying(std::move(underlying))
        , owned(Account(start, taxes))
        , shared(nullptr)
        , policy(policy)
        , rules(rules)
        , brokerFees(brokerFees)
        , state(start.Shares(this->underlying) > 0 ? WheelState::HoldingShares : WheelState::Cash)
    {
        really_assert(!this->underlying.empty());
        really_assert(WritesNothingYet(start, this->underlying));
    }

    Projection::Projection(std::string underlying, Account& shared, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees)
        : underlying(std::move(underlying))
        , owned(std::nullopt)
        , shared(&shared)
        , policy(policy)
        , rules(rules)
        , brokerFees(brokerFees)
        , state(shared.Holdings().Shares(this->underlying) > 0 ? WheelState::HoldingShares : WheelState::Cash)
    {
        really_assert(!this->underlying.empty());
        really_assert(WritesNothingYet(shared.Holdings(), this->underlying));
    }

    std::vector<ProjectionRow> Projection::Over(std::span<const ProjectionPeriod> periods)
    {
        std::vector<ProjectionRow> rows;
        rows.reserve(periods.size());

        for (const auto& period : periods)
        {
            const std::array marks{ SpotPrice{ underlying, period.spotAtExpiry } };

            Write(period);
            rows.push_back(Settle(period, marks));
        }

        return rows;
    }

    const std::string& Projection::Underlying() const
    {
        return underlying;
    }

    WheelState Projection::State() const
    {
        return state;
    }

    const Portfolio& Projection::Holdings() const
    {
        return Books().Holdings();
    }

    const CompoundingLedger& Projection::Ledger() const
    {
        return Books().Ledger();
    }

    Account& Projection::Books()
    {
        return owned.has_value() ? *owned : *shared;
    }

    const Account& Projection::Books() const
    {
        return owned.has_value() ? *owned : *shared;
    }

    void Projection::Write(const ProjectionPeriod& period)
    {
        really_assert(!open.has_value());

        auto& portfolio = Books().Holdings();
        const auto currency = portfolio.GetCurrency();

        really_assert(period.opened <= period.expiry);
        really_assert(period.premiumPerShare >= 0.0);
        really_assert(period.strike.GetCurrency() == currency);
        really_assert(period.spotAtExpiry.GetCurrency() == currency);
        really_assert(state == WheelState::Cash || state == WheelState::HoldingShares);

        const auto writingPut = state == WheelState::Cash;
        const core::OptionContract contract(underlying, period.expiry, period.strike, writingPut ? core::OptionType::Put : core::OptionType::Call);
        const Movements nothing{ 0, core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency) };

        open = OpenCycle{ contract, nothing, period.assessed };
        open->movements.contracts = writingPut
                                        ? policy.ContractsFor(portfolio.FreeCash(), contract.Notional())
                                        : portfolio.UncoveredShares(underlying) / static_cast<std::int64_t>(contract.Multiplier());

        if (open->movements.contracts == 0)
            return;

        if (writingPut)
            portfolio.SellPut(contract, open->movements.contracts);
        else
            portfolio.SellCall(contract, open->movements.contracts);

        MoveOn(writingPut ? WheelEvent::SellPut : WheelEvent::SellCall);

        const auto received = PerContract(period.premiumPerShare, contract, currency) * open->movements.contracts;
        portfolio.Receive(received);
        Books().Ledger().Post(period.opened, CashflowKind::Premium, received);
        open->movements.premium += received;

        const auto openingFee = brokerFees.For(costs::Trade::Options(open->movements.contracts));
        portfolio.Pay(openingFee);
        Books().Ledger().Post(period.opened, CashflowKind::Fee, -openingFee);
        open->movements.fees += openingFee;
    }

    ProjectionRow Projection::Settle(const ProjectionPeriod& period, std::span<const SpotPrice> marks)
    {
        really_assert(open.has_value());
        really_assert(open->contract.Expiry() == period.expiry && open->contract.Strike() == period.strike);

        const auto row = Resolve(period, marks);
        open.reset();

        return row;
    }

    ProjectionRow Projection::Resolve(const ProjectionPeriod& period, std::span<const SpotPrice> marks)
    {
        auto& portfolio = Books().Holdings();
        auto& ledger = Books().Ledger();
        const auto currency = portfolio.GetCurrency();
        const auto contract = open->contract;

        if (open->movements.contracts == 0)
        {
            Assess(period, period.expiry);
            return RowFor(period, marks);
        }

        if (Rolls(period))
        {
            Assess(period, period.observed->on);

            const auto paid = PerContract(period.observed->valuePerShare, contract, currency) * open->movements.contracts;
            portfolio.Close(contract);
            portfolio.Pay(paid);
            ledger.Post(period.observed->on, CashflowKind::Premium, -paid);
            open->movements.premium -= paid;

            const auto closingFee = brokerFees.For(costs::Trade::Options(open->movements.contracts));
            portfolio.Pay(closingFee);
            ledger.Post(period.observed->on, CashflowKind::Fee, -closingFee);
            open->movements.fees += closingFee;

            MoveOn(WheelEvent::Closed);
            Assess(period, period.expiry);
            return RowFor(period, marks);
        }

        Assess(period, period.expiry);

        if (pricing::AssignmentModel::AtExpiry(contract, period.spotAtExpiry) == pricing::AssignmentOutcome::NotAssigned)
        {
            portfolio.Close(contract);
            MoveOn(WheelEvent::ExpiredWorthless);
            return RowFor(period, marks);
        }

        const auto settlement = contract.Notional() * open->movements.contracts;
        portfolio.Assign(contract);
        open->movements.assignment = contract.Type() == core::OptionType::Put ? -settlement : settlement;
        ledger.Post(period.expiry, CashflowKind::Assignment, open->movements.assignment);

        const auto settlementFee = brokerFees.For(costs::Trade::Shares(SharesUnder(contract, open->movements.contracts)));
        portfolio.Pay(settlementFee);
        ledger.Post(period.expiry, CashflowKind::Fee, -settlementFee);
        open->movements.fees += settlementFee;

        MoveOn(WheelEvent::Assigned);
        return RowFor(period, marks);
    }

    void Projection::Assess(const ProjectionPeriod& period, const core::Date& upTo)
    {
        if (!open->assessment.has_value() || open->assessment->on > upTo)
            return;

        auto& portfolio = Books().Holdings();

        really_assert(open->assessment->on >= period.opened && open->assessment->on <= period.expiry);
        really_assert(open->assessment->spot.GetCurrency() == portfolio.GetCurrency());

        const auto on = open->assessment->on;
        const auto charge = Books().Taxes().Charge(AssetsOf(portfolio, underlying, open->assessment->spot));
        open->assessment.reset();

        if (charge.Cents() == 0)
            return;

        portfolio.Pay(charge);
        Books().Ledger().Post(on, CashflowKind::Tax, -charge);
        open->movements.tax += charge;
    }

    ProjectionRow Projection::RowFor(const ProjectionPeriod& period, std::span<const SpotPrice> marks) const
    {
        const auto& portfolio = Books().Holdings();
        const auto& movements = open->movements;

        return { period.expiry, state, movements.contracts, movements.premium, movements.fees, movements.assignment, movements.tax, portfolio.Cash(), portfolio.Shares(underlying), Equity(portfolio, marks) };
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
