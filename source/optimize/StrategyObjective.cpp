#include "source/optimize/StrategyObjective.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/sim/MonteCarlo.hpp"
#include "source/sim/Rng.hpp"
#include "source/strategy/Projection.hpp"
#include "source/strategy/ReinvestmentPolicy.hpp"
#include <algorithm>

namespace fin::optimize
{
    namespace
    {
        constexpr double differenceStep = 0.0625;
    }

    StrategyObjective::StrategyObjective(const WheelExperiment& experiment, const SettingRange& range, const RiskLimits& limits)
        : experiment(experiment)
        , range(range)
        , limits(limits)
    {
        really_assert(IsWellFormed(experiment));
        really_assert(IsWellFormed(range));
        really_assert(IsWellFormed(limits));
    }

    const SettingRange& StrategyObjective::Range() const
    {
        return range;
    }

    const RiskLimits& StrategyObjective::Limits() const
    {
        return limits;
    }

    sim::OutcomeDistribution StrategyObjective::Outcome(const StrategySetting& setting) const
    {
        really_assert(IsWellFormed(setting));

        auto terms = experiment.terms;
        terms.targetDelta = setting.targetDelta;

        auto triggers = experiment.triggers;
        triggers.deltaLimit = setting.rollDeltaLimit;

        const strategy::Projection wheel(experiment.underlying, experiment.start, strategy::ReinvestmentPolicy::WithCashBuffer(setting.cashBufferBasisPoints), strategy::RollRules(triggers), experiment.fees, experiment.taxes);
        const sim::MonteCarlo simulation(wheel, sim::WheelScript(terms), sim::GbmPathGenerator(experiment.underlyingModel));
        sim::Rng rng(experiment.seed);

        return simulation.Over(experiment.paths, rng);
    }

    double StrategyObjective::GrowthRate(const StrategySetting& setting) const
    {
        return Outcome(setting).MeanGrowthRate();
    }

    double StrategyObjective::Cost(const Point& at) const
    {
        const auto outcome = Outcome(SettingAt(range, at));

        return Penalty(limits, ExcessOver(limits, outcome)) - outcome.MeanGrowthRate();
    }

    Point StrategyObjective::Gradient(const Point& at) const
    {
        const auto within = ClampedToTheUnitBox(at);
        Point gradient{};

        for (std::size_t coordinate = 0; coordinate != dimensions; ++coordinate)
        {
            auto above = within;
            auto below = within;
            above[coordinate] = std::min(within[coordinate] + differenceStep, 1.0);
            below[coordinate] = std::max(within[coordinate] - differenceStep, 0.0);

            gradient[coordinate] = (Cost(above) - Cost(below)) / (above[coordinate] - below[coordinate]);
        }

        return gradient;
    }

    bool IsWellFormed(const WheelExperiment& experiment)
    {
        return !experiment.underlying.empty() &&
               strategy::IsWellFormed(experiment.triggers) &&
               sim::IsWellFormed(experiment.terms) &&
               experiment.paths > 0 &&
               experiment.start.ShortPositions().empty();
    }
}
