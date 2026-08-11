#include "source/sim/MonteCarlo.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <cmath>
#include <span>
#include <utility>

namespace fin::sim
{
    namespace
    {
        double CompoundAnnualGrowth(const core::Money& opening, const core::Money& terminal, double years)
        {
            return std::pow(terminal.ToDouble() / opening.ToDouble(), 1.0 / years) - 1.0;
        }
    }

    MonteCarlo::MonteCarlo(const strategy::Projection& wheel, const WheelScript& script, const GbmPathGenerator& generator)
        : wheel(wheel)
        , script(script)
        , generator(generator)
        , schedule(script.Schedule())
    {
        really_assert(wheel.Ledger().Entries().empty());
    }

    OutcomeDistribution MonteCarlo::Over(std::int32_t paths, Rng& rng) const
    {
        really_assert(paths > 0);

        std::vector<PathOutcome> outcomes;
        outcomes.reserve(static_cast<std::size_t>(paths));

        for (std::int32_t path = 0; path != paths; ++path)
            outcomes.push_back(Once(rng));

        return OutcomeDistribution::Of(std::move(outcomes));
    }

    PathOutcome MonteCarlo::Once(Rng& rng) const
    {
        const auto path = generator.Along(schedule, rng);
        auto run = wheel;

        const auto opening = strategy::Equity(run.Holdings(), run.Underlying(), path.Closes().front());
        really_assert(opening.Cents() > 0);

        auto peak = opening;
        auto equity = opening;
        std::int32_t assignments = 0;
        auto drawdown = 0.0;

        for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
        {
            const auto period = script.PeriodFor(cycle, run.State(), path);

            for (const auto& row : run.Over(std::span<const strategy::ProjectionPeriod>(&period, 1)))
            {
                equity = row.equity;
                peak = std::max(peak, equity);
                drawdown = std::max(drawdown, (peak - equity).ToDouble() / peak.ToDouble());

                if (row.assignment.Cents() != 0)
                    ++assignments;
            }
        }

        return { .terminalEquity = equity,
            .growthRate = CompoundAnnualGrowth(opening, equity, script.Horizon()),
            .positions = script.Cycles(),
            .assignments = assignments,
            .maximumDrawdown = drawdown };
    }
}
