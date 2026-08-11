#pragma once

#include "source/core/Date.hpp"
#include "source/core/Delta.hpp"
#include "source/core/Money.hpp"
#include "source/costs/BrokerFees.hpp"
#include "source/strategy/CompoundingLedger.hpp"
#include "source/strategy/Portfolio.hpp"
#include "source/strategy/ReinvestmentPolicy.hpp"
#include "source/strategy/RollRules.hpp"
#include "source/strategy/WheelEngine.hpp"
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

    struct ProjectionPeriod
    {
        core::Date opened;
        core::Date expiry;
        core::Money strike;
        double premiumPerShare;
        core::Money spotAtExpiry;
        std::optional<RollObservation> observed;
    };

    struct ProjectionRow
    {
        core::Date on;
        WheelState state;
        std::int64_t contracts;
        core::Money premium;
        core::Money fees;
        core::Money assignment;
        core::Money cash;
        std::int64_t shares;
        core::Money equity;
    };

    class Projection
    {
    public:
        Projection(std::string underlying, const Portfolio& start, const ReinvestmentPolicy& policy, const RollRules& rules, const costs::BrokerFees& brokerFees);

        std::vector<ProjectionRow> Over(std::span<const ProjectionPeriod> periods);

        WheelState State() const;
        const Portfolio& Holdings() const;
        const CompoundingLedger& Ledger() const;

    private:
        ProjectionRow Advance(const ProjectionPeriod& period);
        bool Rolls(const ProjectionPeriod& period) const;
        void MoveOn(WheelEvent event);

        std::string underlying;
        Portfolio portfolio;
        ReinvestmentPolicy policy;
        RollRules rules;
        costs::BrokerFees brokerFees;
        CompoundingLedger ledger;
        WheelState state;
    };
}
