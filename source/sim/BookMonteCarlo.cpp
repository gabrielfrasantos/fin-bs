#include "source/sim/BookMonteCarlo.hpp"
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

        std::vector<strategy::SpotPrice> MarksOn(std::span<const std::string> underlyings, std::span<const estimation::PriceSeries> paths, std::size_t observation)
        {
            std::vector<strategy::SpotPrice> marks;
            marks.reserve(underlyings.size());

            for (std::size_t wheel = 0; wheel != underlyings.size(); ++wheel)
                marks.push_back({ underlyings[wheel], paths[wheel].Closes()[observation] });

            return marks;
        }
    }

    BookMonteCarlo::BookMonteCarlo(const strategy::Book& book, const BookScript& script, std::vector<GbmPathGenerator> generators)
        : book(book)
        , script(script)
        , generators(std::move(generators))
        , schedule(script.Schedule())
    {
        really_assert(book.Ledger().Entries().empty());
        really_assert(this->generators.size() == book.Underlyings().size());
        really_assert(script.Underlyings().size() == book.Underlyings().size());
    }

    OutcomeDistribution BookMonteCarlo::Over(std::int32_t paths, Rng& rng) const
    {
        really_assert(paths > 0);

        std::vector<PathOutcome> outcomes;
        outcomes.reserve(static_cast<std::size_t>(paths));

        for (std::int32_t path = 0; path != paths; ++path)
            outcomes.push_back(Once(rng));

        return OutcomeDistribution::Of(std::move(outcomes));
    }

    OutcomeDistribution BookMonteCarlo::Over(std::int32_t paths, Rng& rng, RunObserver& observer) const
    {
        really_assert(paths > 0);

        std::vector<PathOutcome> outcomes;
        outcomes.reserve(static_cast<std::size_t>(paths));

        for (std::int32_t path = 0; path != paths && observer.Continues(); ++path)
        {
            outcomes.push_back(Once(rng));
            observer.Completed(path + 1, paths);
        }

        really_assert(!outcomes.empty());

        return OutcomeDistribution::Of(std::move(outcomes));
    }

    PathOutcome BookMonteCarlo::Once(Rng& rng) const
    {
        std::vector<estimation::PriceSeries> paths;
        paths.reserve(generators.size());

        for (const auto& generator : generators)
            paths.push_back(generator.Along(schedule, rng));

        auto run = book;
        const auto underlyings = script.Underlyings();

        const auto opening = Equity(run.Holdings(), MarksOn(underlyings, paths, 0));
        really_assert(opening.Cents() > 0);

        auto peak = opening;
        auto equity = opening;
        std::int32_t assignments = 0;
        auto drawdown = 0.0;
        std::vector<core::Money> equityByCycle{ opening };
        equityByCycle.reserve(static_cast<std::size_t>(script.Cycles()) + 1u);

        for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
        {
            if (const auto assessed = script.AssessmentBefore(cycle, paths))
                run.Assess(*assessed);

            for (const auto& row : run.Over(script.CycleFor(cycle, run, paths)))
            {
                equity = row.row.equity;

                if (row.row.assignment.Cents() != 0)
                    ++assignments;
            }

            peak = std::max(peak, equity);
            drawdown = std::max(drawdown, (peak - equity).ToDouble() / peak.ToDouble());
            equityByCycle.push_back(equity);
        }

        return { .terminalEquity = equity,
            .growthRate = CompoundAnnualGrowth(opening, equity, script.Horizon()),
            .positions = script.Cycles() * static_cast<std::int32_t>(underlyings.size()),
            .assignments = assignments,
            .maximumDrawdown = drawdown,
            .equityByCycle = std::move(equityByCycle) };
    }
}
