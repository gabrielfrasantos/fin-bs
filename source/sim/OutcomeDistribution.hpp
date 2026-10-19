#pragma once

#include "source/core/Money.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace fin::sim
{
    struct PathOutcome
    {
        core::Money terminalEquity;
        double growthRate;
        std::int32_t positions;
        std::int32_t assignments;
        std::int32_t cyclesInDebt;
        double maximumDrawdown;
        std::vector<core::Money> equityByCycle;
    };

    class OutcomeDistribution
    {
    public:
        static OutcomeDistribution Of(std::vector<PathOutcome> outcomes);

        std::size_t Paths() const;
        std::span<const PathOutcome> Outcomes() const;
        const PathOutcome& At(std::int32_t percentile) const;
        core::Money Equity(std::int32_t percentile) const;
        double GrowthRate(std::int32_t percentile) const;
        core::Money MeanEquity() const;
        double MeanGrowthRate() const;
        double WorstDrawdown() const;
        double AssignmentFrequency() const;
        double DebtFrequency() const;

    private:
        explicit OutcomeDistribution(std::vector<PathOutcome>&& ordered);

        std::vector<PathOutcome> ordered;
    };
}
