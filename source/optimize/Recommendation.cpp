#include "source/optimize/Recommendation.hpp"

namespace fin::optimize
{
    namespace
    {
        constexpr std::int32_t median = 50;
    }

    bool IsFeasible(const Recommendation& recommendation)
    {
        return !recommendation.drawdownBinds && !recommendation.assignmentBinds;
    }

    Recommendation Recommend(const StrategySetting& start, const StrategyObjective& objective, const StrikeSizingOptimizer& optimizer)
    {
        const auto descent = optimizer.From(PointOf(objective.Range(), start), objective);
        const auto setting = SettingAt(objective.Range(), descent.at);
        const auto outcome = objective.Outcome(setting);
        const auto excess = ExcessOver(objective.Limits(), outcome);

        return { .setting = setting,
            .growthRate = outcome.MeanGrowthRate(),
            .medianEquity = outcome.Equity(median),
            .worstDrawdown = outcome.WorstDrawdown(),
            .assignmentFrequency = outcome.AssignmentFrequency(),
            .drawdownBinds = Binds(excess.drawdown),
            .assignmentBinds = Binds(excess.assignmentFrequency) };
    }
}
