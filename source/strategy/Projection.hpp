#pragma once

#include "source/core/Date.hpp"
#include "source/core/Delta.hpp"
#include "source/core/Money.hpp"
#include "source/costs/BrokerFees.hpp"
#include "source/strategy/Account.hpp"
#include "source/strategy/CompoundingLedger.hpp"
#include "source/strategy/Portfolio.hpp"
#include "source/strategy/ReinvestmentPolicy.hpp"
#include "source/strategy/RollRules.hpp"
#include "source/strategy/WheelEngine.hpp"
#include "source/tax/Box3TaxModel.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace fin::strategy
{
    struct RollObservation
    {
        core::Date on;
        double valuePerShare;
        core::Delta delta;
    };

    struct TaxObservation
    {
        core::Date on;
        core::Money spot;
    };

    struct ProjectionPeriod
    {
        core::Date opened;
        core::Date expiry;
        core::Money spotAtOpen;
        core::Money strike;
        double premiumPerShare;
        core::Money spotAtExpiry;
        std::optional<RollObservation> observed;
        std::optional<TaxObservation> assessed;
        double dividendPerShare;
        std::int32_t dividendWithholdingBasisPoints;
    };

    struct ProjectionRow
    {
        core::Date on;
        WheelState state;
        std::int64_t contracts;
        core::Money premium;
        core::Money fees;
        core::Money assignment;
        core::Money dividend;
        core::Money tax;
        core::Money cash;
        std::int64_t shares;
        core::Money equity;

        bool operator==(const ProjectionRow& other) const = default;
    };

    class Projection
    {
    public:
        Projection(std::string underlying, const Portfolio& start, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees, const tax::Box3TaxModel& taxes);
        Projection(std::string underlying, Account& shared, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees);

        std::vector<ProjectionRow> Over(std::span<const ProjectionPeriod> periods);

        void Write(const ProjectionPeriod& period, const core::Money& openingEquity);
        ProjectionRow Settle(const ProjectionPeriod& period, std::span<const SpotPrice> marks);

        const std::string& Underlying() const;
        WheelState State() const;
        const Portfolio& Holdings() const;
        const CompoundingLedger& Ledger() const;

    private:
        struct Movements
        {
            std::int64_t contracts;
            core::Money premium;
            core::Money fees;
            core::Money assignment;
            core::Money dividend;
            core::Money tax;
        };

        struct OpenCycle
        {
            core::OptionContract contract;
            Movements movements;
            std::optional<TaxObservation> assessment;
            std::int64_t sharesHeld;
            core::Money openingCash;
        };

        Account& Books();
        const Account& Books() const;
        void CreditDividend(const ProjectionPeriod& period);
        void Resolve(const ProjectionPeriod& period);
        void Assess(const ProjectionPeriod& period, const core::Date& upTo);
        ProjectionRow RowFor(const ProjectionPeriod& period, std::span<const SpotPrice> marks) const;
        bool Rolls(const ProjectionPeriod& period) const;
        void MoveOn(WheelEvent event);

        std::string underlying;
        std::optional<Account> owned;
        Account* shared;
        ReinvestmentPolicy policy;
        RollRules rules;
        costs::BrokerFees brokerFees;
        WheelState state;
        std::optional<OpenCycle> open;
    };
}
