#include "source/strategy/Book.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/strategy/CollateralAllocation.hpp"
#include <algorithm>
#include <utility>

namespace fin::strategy
{
    namespace
    {
        tax::Box3Assets AssetsOf(const Portfolio& portfolio, std::span<const SpotPrice> prices)
        {
            const auto nothing = core::Money::Zero(portfolio.GetCurrency());
            const auto equity = std::max(Equity(portfolio, prices), nothing);
            const auto deposits = std::min(std::max(portfolio.Cash(), nothing), equity);

            return { deposits, equity - deposits };
        }
    }

    Book::Book(std::vector<std::string> underlyings, const Portfolio& start, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees, const tax::Box3TaxModel& taxes)
        : account(start, taxes)
        , underlyings(std::move(underlyings))
        , policy(policy)
        , rules(rules)
        , brokerFees(brokerFees)
    {
        really_assert(!this->underlyings.empty());

        for (const auto& underlying : this->underlyings)
        {
            really_assert(!underlying.empty());
            really_assert(std::count(this->underlyings.begin(), this->underlyings.end(), underlying) == 1);
        }
    }

    std::vector<BookRow> Book::Over(std::span<const BookPeriod> cycle)
    {
        really_assert(cycle.size() == underlyings.size());

        const auto opened = cycle.front().period.opened;
        const auto expiry = cycle.front().period.expiry;
        const auto openingCash = account.Holdings().Cash();

        std::vector<CollateralClaim> claims;
        std::vector<SpotPrice> marks;
        claims.reserve(cycle.size());
        marks.reserve(cycle.size());

        for (const auto& entry : cycle)
        {
            really_assert(Carries(entry.underlying));
            really_assert(!entry.period.assessed.has_value());
            really_assert(entry.period.opened == opened && entry.period.expiry == expiry);
            really_assert(std::count_if(cycle.begin(), cycle.end(), [&entry](const BookPeriod& other)
                              {
                                  return other.underlying == entry.underlying;
                              }) == 1);

            claims.push_back({ .fromCash = StateOf(entry.underlying) == WheelState::Cash,
                .premiumPerShare = entry.period.premiumPerShare,
                .strike = entry.period.strike });

            marks.push_back({ entry.underlying, entry.period.spotAtExpiry });
        }

        std::vector<Projection> wheels;
        wheels.reserve(cycle.size());

        for (const auto& entry : cycle)
            wheels.emplace_back(entry.underlying, account, policy, rules, brokerFees);

        for (const auto claim : ServiceOrder(claims))
            wheels[claim].Write(cycle[claim].period);

        AccrueDebitInterest(account, brokerFees, openingCash, opened, expiry);

        std::vector<BookRow> rows;
        rows.reserve(cycle.size());

        for (std::size_t claim = 0; claim != cycle.size(); ++claim)
            rows.push_back({ cycle[claim].underlying, wheels[claim].Settle(cycle[claim].period, marks) });

        return rows;
    }

    core::Money Book::Assess(const BookAssessment& assessment)
    {
        const auto charge = account.Taxes().Charge(AssetsOf(account.Holdings(), assessment.prices));

        if (charge.Cents() == 0)
            return charge;

        account.Holdings().Pay(charge);
        account.Ledger().Post(assessment.on, CashflowKind::Tax, -charge);

        return charge;
    }

    std::span<const std::string> Book::Underlyings() const
    {
        return underlyings;
    }

    WheelState Book::StateOf(const std::string& underlying) const
    {
        really_assert(Carries(underlying));

        return account.Holdings().Shares(underlying) > 0 ? WheelState::HoldingShares : WheelState::Cash;
    }

    const Portfolio& Book::Holdings() const
    {
        return account.Holdings();
    }

    const CompoundingLedger& Book::Ledger() const
    {
        return account.Ledger();
    }

    const Account& Book::Holds() const
    {
        return account;
    }

    bool Book::Carries(const std::string& underlying) const
    {
        return std::find(underlyings.begin(), underlyings.end(), underlying) != underlyings.end();
    }
}
