#include "source/strategy/Projection.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/core/OptionContract.hpp"
#include "source/costs/Trade.hpp"
#include "source/pricing/AssignmentModel.hpp"
#include "source/tax/Box3Parameters.hpp"
#include <algorithm>
#include <array>
#include <utility>

namespace fin::strategy
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;

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
            const auto nothing = core::Money::Zero(portfolio.GetCurrency());
            const auto equity = std::max(Equity(portfolio, underlying, spot), nothing);
            const auto deposits = std::min(std::max(portfolio.Cash(), nothing), equity);

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

            Write(period, Equity(Books().Holdings(), underlying, period.spotAtOpen));
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

    void Projection::Write(const ProjectionPeriod& period, const core::Money& openingEquity)
    {
        really_assert(!open.has_value());

        auto& portfolio = Books().Holdings();
        const auto currency = portfolio.GetCurrency();

        really_assert(period.opened <= period.expiry);
        really_assert(period.spotAtOpen.GetCurrency() == currency);
        really_assert(period.spotAtOpen.Cents() >= 0);
        really_assert(openingEquity.GetCurrency() == currency);
        really_assert(period.premiumPerShare >= 0.0);
        really_assert(period.dividendPerShare >= 0.0);
        really_assert(period.dividendWithholdingBasisPoints >= 0 && period.dividendWithholdingBasisPoints <= basisPointsPerUnit);
        really_assert(period.strike.GetCurrency() == currency);
        really_assert(period.spotAtExpiry.GetCurrency() == currency);
        really_assert(state == WheelState::Cash || state == WheelState::HoldingShares);

        const auto writingPut = state == WheelState::Cash;
        const core::OptionContract contract(underlying, period.expiry, period.strike, writingPut ? core::OptionType::Put : core::OptionType::Call);
        const Movements nothing{ 0, core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency), core::Money::Zero(currency) };

        open = OpenCycle{ contract, nothing, period.assessed, portfolio.Shares(underlying), portfolio.Cash() };
        open->movements.contracts = writingPut
                                        ? policy.ContractsFor(portfolio.FreeCash(), contract.Notional(), openingEquity)
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

        CreditDividend(period);
        Resolve(period);

        if (owned.has_value())
            AccrueDebitInterest(*owned, brokerFees, open->openingCash, period.opened, period.expiry);

        const auto row = RowFor(period, marks);
        open.reset();

        return row;
    }

    void Projection::CreditDividend(const ProjectionPeriod& period)
    {
        if (open->sharesHeld == 0 || period.dividendPerShare <= 0.0)
            return;

        auto& portfolio = Books().Holdings();
        const auto gross = core::Money::FromAmount(period.dividendPerShare * static_cast<double>(open->sharesHeld), portfolio.GetCurrency());
        const auto net = gross - tax::AtRate(gross, period.dividendWithholdingBasisPoints);

        if (net.Cents() <= 0)
            return;

        portfolio.Receive(net);
        Books().Ledger().Post(period.expiry, CashflowKind::Dividend, net);
        open->movements.dividend += net;
    }

    void Projection::Resolve(const ProjectionPeriod& period)
    {
        auto& portfolio = Books().Holdings();
        auto& ledger = Books().Ledger();
        const auto currency = portfolio.GetCurrency();
        const auto contract = open->contract;

        if (open->movements.contracts == 0)
        {
            Assess(period, period.expiry);
            return;
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
            return;
        }

        Assess(period, period.expiry);

        if (pricing::AssignmentModel::AtExpiry(contract, period.spotAtExpiry) == pricing::AssignmentOutcome::NotAssigned)
        {
            portfolio.Close(contract);
            MoveOn(WheelEvent::ExpiredWorthless);
            return;
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

        return { period.expiry, state, movements.contracts, movements.premium, movements.fees, movements.assignment, movements.dividend, movements.tax, portfolio.Cash(), portfolio.Shares(underlying), Equity(portfolio, marks) };
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
