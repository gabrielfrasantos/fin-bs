#pragma once

#include "source/core/Money.hpp"
#include "source/optimize/StrategyObjective.hpp"
#include "source/optimize/StrikeSizingOptimizer.hpp"

namespace fin::optimize
{
    struct Recommendation
    {
        StrategySetting setting;
        double growthRate;
        core::Money medianEquity;
        double worstDrawdown;
        double assignmentFrequency;
        bool drawdownBinds;
        bool assignmentBinds;
    };

    bool IsFeasible(const Recommendation& recommendation);
    Recommendation Recommend(const StrategySetting& start, const StrategyObjective& objective, const StrikeSizingOptimizer& optimizer);
}
