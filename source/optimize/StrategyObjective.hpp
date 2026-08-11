#pragma once

#include "source/costs/BrokerFees.hpp"
#include "source/optimize/Objective.hpp"
#include "source/optimize/RiskLimits.hpp"
#include "source/optimize/StrategySetting.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include "source/sim/WheelScript.hpp"
#include "source/strategy/Portfolio.hpp"
#include "source/strategy/RollRules.hpp"
#include "source/tax/Box3TaxModel.hpp"
#include <cstdint>
#include <string>

namespace fin::optimize
{
    struct WheelExperiment
    {
        std::string underlying;
        strategy::Portfolio start;
        strategy::RollTriggers triggers;
        costs::BrokerFees fees;
        tax::Box3TaxModel taxes;
        sim::WheelTerms terms;
        sim::GbmParameters underlyingModel;
        std::int32_t paths;
        std::uint64_t seed;
    };

    class StrategyObjective
        : public Objective
    {
    public:
        StrategyObjective(const WheelExperiment& experiment, const SettingRange& range, const RiskLimits& limits);

        const SettingRange& Range() const;
        const RiskLimits& Limits() const;
        sim::OutcomeDistribution Outcome(const StrategySetting& setting) const;
        double GrowthRate(const StrategySetting& setting) const;
        double Cost(const Point& at) const override;
        Point Gradient(const Point& at) const override;

    private:
        WheelExperiment experiment;
        SettingRange range;
        RiskLimits limits;
    };

    bool IsWellFormed(const WheelExperiment& experiment);
}
