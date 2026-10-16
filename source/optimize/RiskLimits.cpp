#include "source/optimize/RiskLimits.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::optimize
{
    bool IsWellFormed(const RiskLimits& limits)
    {
        return limits.maximumDrawdown >= 0.0 &&
               limits.maximumDrawdown <= 1.0 &&
               limits.maximumAssignmentFrequency >= 0.0 &&
               limits.maximumAssignmentFrequency <= 1.0 &&
               limits.penaltyWeight >= 0.0;
    }

    RiskExcess ExcessOver(const RiskLimits& limits, const sim::OutcomeDistribution& outcome)
    {
        really_assert(IsWellFormed(limits));

        return { .drawdown = outcome.WorstDrawdown() - limits.maximumDrawdown,
            .assignmentFrequency = outcome.AssignmentFrequency() - limits.maximumAssignmentFrequency };
    }

    bool Binds(double excess)
    {
        return excess >= 0.0;
    }

    bool IsFeasible(const RiskExcess& excess)
    {
        return excess.drawdown <= 0.0 && excess.assignmentFrequency <= 0.0;
    }

    double Penalty(const RiskLimits& limits, const RiskExcess& excess)
    {
        really_assert(IsWellFormed(limits));

        return limits.penaltyWeight * (std::max(excess.drawdown, 0.0) + std::max(excess.assignmentFrequency, 0.0));
    }
}
