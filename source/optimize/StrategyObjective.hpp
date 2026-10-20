#pragma once

#include "source/costs/BrokerFees.hpp"
#include "source/optimize/Objective.hpp"
#include "source/optimize/RiskLimits.hpp"
#include "source/optimize/StrategySetting.hpp"
#include "source/sim/BookScript.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include "source/strategy/Portfolio.hpp"
#include "source/strategy/RollRules.hpp"
#include "source/tax/Box3TaxModel.hpp"
#include <cstdint>

namespace fin::optimize
{
    struct BookExperiment
    {
        strategy::Portfolio start;
        strategy::RollTriggers triggers;
        std::int32_t maxCollateralPerNameBasisPoints;
        costs::BrokerFees fees;
        tax::Box3TaxModel taxes;
        sim::BookTerms terms;
        std::int32_t paths;
        std::uint64_t seed;
    };

    class StrategyObjective
        : public Objective
    {
    public:
        StrategyObjective(const BookExperiment& experiment, const SettingRange& range, const RiskLimits& limits);

        const SettingRange& Range() const;
        const RiskLimits& Limits() const;
        sim::OutcomeDistribution Outcome(const StrategySetting& setting) const;
        double GrowthRate(const StrategySetting& setting) const;
        double Cost(const Point& at) const override;
        Point Gradient(const Point& at) const override;

    private:
        BookExperiment experiment;
        SettingRange range;
        RiskLimits limits;
    };

    bool IsWellFormed(const BookExperiment& experiment);
}
