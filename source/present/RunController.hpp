#pragma once

#include "source/core/Date.hpp"
#include "source/costs/FeeSchedule.hpp"
#include "source/estimation/PriceSeries.hpp"
#include "source/io/ScenarioLoader.hpp"
#include "source/optimize/Recommendation.hpp"
#include "source/optimize/RiskLimits.hpp"
#include "source/optimize/StrategySetting.hpp"
#include "source/optimize/StrikeSizingOptimizer.hpp"
#include "source/present/CalibrationRow.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include "source/sim/RunObserver.hpp"
#include "source/strategy/Book.hpp"
#include "source/strategy/CompoundingLedger.hpp"
#include "source/tax/Box3Parameters.hpp"
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fin::present
{
    enum class RunKind : std::uint8_t
    {
        projection,
        simulation,
        search,
        backtest
    };

    enum class RunState : std::uint8_t
    {
        idle,
        running,
        completed,
        cancelled,
        failed
    };

    struct RunProgress
    {
        std::int32_t completed;
        std::int32_t total;

        bool operator==(const RunProgress& other) const = default;
    };

    struct SearchTerms
    {
        optimize::SettingRange range;
        optimize::RiskLimits limits;
        optimize::DescentTerms descent;
    };

    struct NamedHistory
    {
        std::string underlying;
        estimation::PriceSeries history;
    };

    struct RunInputs
    {
        io::Scenario scenario;
        costs::FeeSchedule fees;
        tax::Box3Parameters taxes;
        SearchTerms search;
        std::vector<NamedHistory> histories;
    };

    struct RunOutcome
    {
        RunKind kind;
        std::vector<core::Date> boundaries;
        std::vector<core::Money> equity;
        std::vector<strategy::BookRow> rows;
        strategy::CompoundingLedger ledger;
        std::optional<sim::OutcomeDistribution> distribution;
        std::optional<optimize::Recommendation> recommendation;
        std::vector<CalibrationRow> calibration;
    };

    class RunController
        : public sim::RunObserver
    {
    public:
        RunController() = default;

        void Start(const RunInputs& inputs, RunKind kind);
        void Cancel();

        RunState State() const;
        RunProgress Progress() const;
        const std::string& Failure() const;
        const std::optional<RunOutcome>& Result() const;

        void Completed(std::int32_t path, std::int32_t of) override;
        bool Continues() const override;

    private:
        RunOutcome Project(const RunInputs& inputs);
        RunOutcome Simulate(const RunInputs& inputs);
        RunOutcome Search(const RunInputs& inputs);
        RunOutcome Backtest(const RunInputs& inputs);

        std::atomic<RunState> state{ RunState::idle };
        std::atomic<std::int32_t> completed{ 0 };
        std::atomic<std::int32_t> total{ 0 };
        std::atomic<bool> cancelling{ false };
        std::string failure;
        std::optional<RunOutcome> result;
    };

    std::vector<core::Date> CycleBoundaries(const sim::BookTerms& terms);
    SearchTerms SearchAround(const io::Scenario& scenario, std::int32_t maximumDrawdownBasisPoints, std::int32_t maximumAssignmentBasisPoints, std::size_t iterations);
}
