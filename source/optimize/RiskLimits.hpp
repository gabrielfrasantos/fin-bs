#pragma once

#include "source/sim/OutcomeDistribution.hpp"

namespace fin::optimize
{
    struct RiskLimits
    {
        double maximumDrawdown;
        double maximumAssignmentFrequency;
        double penaltyWeight;
    };

    struct RiskExcess
    {
        double drawdown;
        double assignmentFrequency;
    };

    bool IsWellFormed(const RiskLimits& limits);
    RiskExcess ExcessOver(const RiskLimits& limits, const sim::OutcomeDistribution& outcome);
    bool Binds(double excess);
    bool IsFeasible(const RiskExcess& excess);
    double Penalty(const RiskLimits& limits, const RiskExcess& excess);
}
