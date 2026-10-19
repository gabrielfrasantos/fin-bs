#include "source/present/RunController.hpp"
#include "source/costs/BrokerFees.hpp"
#include "source/optimize/StrategyObjective.hpp"
#include "source/sim/BookMonteCarlo.hpp"
#include "source/sim/BookScript.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include "source/sim/Rng.hpp"
#include "source/strategy/Portfolio.hpp"
#include "source/strategy/ReinvestmentPolicy.hpp"
#include "source/tax/Box3TaxModel.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <utility>

namespace fin::present
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;
        constexpr std::int32_t leastDeltaSpread = 100;
        constexpr std::int32_t leastBufferSpread = 1000;
        constexpr double descentLearningRate = 4.0;
        constexpr double penaltyWeight = 1.0;

        struct Band
        {
            std::int32_t lowest;
            std::int32_t highest;
        };

        Band Around(std::int32_t value, std::int32_t least, std::int32_t most, std::int32_t leastSpread)
        {
            const auto lowest = std::max(least, value / 2);

            return { lowest, std::min(most, std::max(value * 2, lowest + leastSpread)) };
        }

        double FractionOf(std::int32_t basisPoints)
        {
            return static_cast<double>(basisPoints) / static_cast<double>(basisPointsPerUnit);
        }

        std::int32_t BasisPointsOf(double fraction)
        {
            return static_cast<std::int32_t>(std::llround(fraction * basisPointsPerUnit));
        }

        std::vector<sim::GbmPathGenerator> MarketsOf(const sim::BookTerms& terms)
        {
            std::vector<sim::GbmPathGenerator> generators;
            generators.reserve(terms.underlyings.size());

            for (const auto& underlying : terms.underlyings)
                generators.push_back(sim::GbmPathGenerator(sim::GbmParameters{ .initialPrice = underlying.initialPrice,
                    .drift = underlying.drift,
                    .volatility = underlying.volatility,
                    .convention = core::DayCountConvention::Act365Fixed }));

            return generators;
        }

        strategy::Book BookOf(const RunInputs& inputs, const sim::BookScript& script)
        {
            return strategy::Book(script.Underlyings(),
                strategy::Portfolio::Funded(inputs.scenario.capital),
                strategy::ReinvestmentPolicy::WithCashBufferAndCap(inputs.scenario.cashBufferBasisPoints, inputs.scenario.maxCollateralPerNameBasisPoints),
                strategy::RollRules(inputs.scenario.triggers),
                costs::BrokerFees(inputs.fees),
                tax::Box3TaxModel(inputs.taxes));
        }

        optimize::StrategySetting SettingOf(const io::Scenario& scenario)
        {
            return { .targetDelta = scenario.terms.targetDelta,
                .cashBufferBasisPoints = scenario.cashBufferBasisPoints,
                .rollDeltaLimit = scenario.triggers.deltaLimit };
        }

        void RefuseUnless(bool held, const char* complaint)
        {
            if (!held)
                throw std::runtime_error(complaint);
        }

        class WatchedObjective
            : public optimize::StrategyObjective
        {
        public:
            WatchedObjective(const optimize::BookExperiment& experiment, const SearchTerms& terms, RunController& controller)
                : optimize::StrategyObjective(experiment, terms.range, terms.limits)
                , controller(controller)
                , iterations(static_cast<std::int32_t>(terms.descent.iterations))
            {}

            double Cost(const optimize::Point& at) const override
            {
                if (controller.Continues())
                    cost = optimize::StrategyObjective::Cost(at);

                return cost;
            }

            optimize::Point Gradient(const optimize::Point& at) const override
            {
                if (!controller.Continues())
                    return optimize::Point{};

                auto gradient = optimize::StrategyObjective::Gradient(at);
                controller.Completed(std::min(++taken, iterations), iterations);

                return gradient;
            }

        private:
            RunController& controller;
            std::int32_t iterations;
            mutable std::int32_t taken{ 0 };
            mutable double cost{ 0.0 };
        };
    }

    void RunController::Start(const RunInputs& inputs, RunKind kind)
    {
        cancelling = false;
        completed = 0;
        total = 0;
        failure.clear();
        result.reset();
        state = RunState::running;

        try
        {
            RefuseUnless(io::IsWellFormed(inputs.scenario), "The scenario does not describe a run that can be made");
            RefuseUnless(costs::IsWellFormed(inputs.fees), "The broker configuration does not describe a fee schedule");
            RefuseUnless(tax::IsWellFormed(inputs.taxes), "The tax configuration does not describe a year of Box 3");
            RefuseUnless(costs::CurrencyOf(inputs.fees) == io::CurrencyOf(inputs.scenario), "The broker configuration is stated in another currency than the scenario");
            RefuseUnless(tax::CurrencyOf(inputs.taxes) == io::CurrencyOf(inputs.scenario), "The tax configuration is stated in another currency than the scenario");

            if (kind == RunKind::projection)
                result = Project(inputs);
            else if (kind == RunKind::simulation)
                result = Simulate(inputs);
            else
                result = Search(inputs);

            state = cancelling ? RunState::cancelled : RunState::completed;
        }
        catch (const std::exception& error)
        {
            result.reset();
            failure = error.what();
            state = RunState::failed;
        }
    }

    void RunController::Cancel()
    {
        cancelling = true;
    }

    RunState RunController::State() const
    {
        return state;
    }

    RunProgress RunController::Progress() const
    {
        return { completed, total };
    }

    const std::string& RunController::Failure() const
    {
        return failure;
    }

    const std::optional<RunOutcome>& RunController::Result() const
    {
        return result;
    }

    void RunController::Completed(std::int32_t path, std::int32_t of)
    {
        completed = path;
        total = of;
    }

    bool RunController::Continues() const
    {
        return !cancelling || completed == 0;
    }

    RunOutcome RunController::Project(const RunInputs& inputs)
    {
        const sim::BookScript script(inputs.scenario.terms);
        const auto schedule = script.Schedule();
        sim::Rng rng(inputs.scenario.seed);

        std::vector<estimation::PriceSeries> paths;
        paths.reserve(inputs.scenario.terms.underlyings.size());

        for (const auto& generator : MarketsOf(inputs.scenario.terms))
            paths.push_back(generator.Along(schedule, rng));

        auto book = BookOf(inputs, script);
        std::vector<strategy::BookRow> rows;
        std::vector<core::Date> boundaries{ inputs.scenario.terms.start };
        std::vector<core::Money> equity{ inputs.scenario.capital };

        for (std::int32_t cycle = 0; cycle != script.Cycles() && Continues(); ++cycle)
        {
            if (const auto assessed = script.AssessmentBefore(cycle, paths))
                book.Assess(*assessed);

            for (const auto& row : book.Over(script.CycleFor(cycle, book, paths)))
                rows.push_back(row);

            boundaries.push_back(inputs.scenario.terms.start.AddDays((cycle + 1) * inputs.scenario.terms.daysPerCycle));
            equity.push_back(rows.back().row.equity);
            Completed(cycle + 1, script.Cycles());
        }

        return { .kind = RunKind::projection,
            .boundaries = std::move(boundaries),
            .equity = std::move(equity),
            .rows = std::move(rows),
            .ledger = book.Ledger(),
            .distribution = std::nullopt,
            .recommendation = std::nullopt };
    }

    RunOutcome RunController::Simulate(const RunInputs& inputs)
    {
        const sim::BookScript script(inputs.scenario.terms);
        const auto book = BookOf(inputs, script);
        const sim::BookMonteCarlo simulation(book, script, MarketsOf(inputs.scenario.terms));
        sim::Rng rng(inputs.scenario.seed);

        return { .kind = RunKind::simulation,
            .boundaries = CycleBoundaries(inputs.scenario.terms),
            .equity = {},
            .rows = {},
            .ledger = book.Ledger(),
            .distribution = simulation.Over(inputs.scenario.paths, rng, *this),
            .recommendation = std::nullopt };
    }

    RunOutcome RunController::Search(const RunInputs& inputs)
    {
        RefuseUnless(optimize::IsWellFormed(inputs.search.range), "The search does not span a band of settings");
        RefuseUnless(optimize::IsWellFormed(inputs.search.limits), "The search does not state limits that can be met");
        RefuseUnless(optimize::IsWellFormed(inputs.search.descent), "The search does not state a descent that can be made");

        const optimize::BookExperiment experiment{ .start = strategy::Portfolio::Funded(inputs.scenario.capital),
            .triggers = inputs.scenario.triggers,
            .maxCollateralPerNameBasisPoints = inputs.scenario.maxCollateralPerNameBasisPoints,
            .fees = costs::BrokerFees(inputs.fees),
            .taxes = tax::Box3TaxModel(inputs.taxes),
            .terms = inputs.scenario.terms,
            .paths = inputs.scenario.paths,
            .seed = inputs.scenario.seed };

        Completed(0, static_cast<std::int32_t>(inputs.search.descent.iterations));

        const WatchedObjective objective(experiment, inputs.search, *this);
        const optimize::StrikeSizingOptimizer optimizer(inputs.search.descent);

        return { .kind = RunKind::search,
            .boundaries = CycleBoundaries(inputs.scenario.terms),
            .equity = {},
            .rows = {},
            .ledger = strategy::CompoundingLedger(io::CurrencyOf(inputs.scenario)),
            .distribution = std::nullopt,
            .recommendation = optimize::Recommend(SettingOf(inputs.scenario), objective, optimizer) };
    }

    std::vector<core::Date> CycleBoundaries(const sim::BookTerms& terms)
    {
        std::vector<core::Date> boundaries;
        boundaries.reserve(static_cast<std::size_t>(terms.cycles) + 1u);

        for (std::int32_t cycle = 0; cycle != terms.cycles + 1; ++cycle)
            boundaries.push_back(terms.start.AddDays(cycle * terms.daysPerCycle));

        return boundaries;
    }

    SearchTerms SearchAround(const io::Scenario& scenario, std::int32_t maximumDrawdownBasisPoints, std::int32_t maximumAssignmentBasisPoints, std::size_t iterations)
    {
        const auto setting = SettingOf(scenario);
        const auto delta = Around(BasisPointsOf(setting.targetDelta.Value()), 1, basisPointsPerUnit - 1, leastDeltaSpread);
        const auto buffer = Around(setting.cashBufferBasisPoints, 0, basisPointsPerUnit, leastBufferSpread);
        const auto roll = Around(BasisPointsOf(setting.rollDeltaLimit.Value()), 1, basisPointsPerUnit, leastDeltaSpread);

        return { .range = optimize::SettingRange{ .lowest = { core::Delta::FromValue(FractionOf(delta.lowest)), buffer.lowest, core::Delta::FromValue(FractionOf(roll.lowest)) },
                     .highest = { core::Delta::FromValue(FractionOf(delta.highest)), buffer.highest, core::Delta::FromValue(FractionOf(roll.highest)) } },
            .limits = optimize::RiskLimits{ .maximumDrawdown = FractionOf(maximumDrawdownBasisPoints),
                .maximumAssignmentFrequency = FractionOf(maximumAssignmentBasisPoints),
                .penaltyWeight = penaltyWeight },
            .descent = optimize::DescentTerms{ .learningRate = descentLearningRate, .iterations = iterations, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 } };
    }
}
