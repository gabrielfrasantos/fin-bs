#pragma once

#include "source/core/Money.hpp"
#include <cstdint>
#include <vector>

namespace fin::sim
{
    struct PathOutcome
    {
        core::Money terminalEquity;
        double growthRate;
        std::int32_t positions;
        std::int32_t assignments;
        double maximumDrawdown;
    };

    class OutcomeDistribution
    {
    public:
        static OutcomeDistribution Of(std::vector<PathOutcome> outcomes);

        std::size_t Paths() const;
        const PathOutcome& At(std::int32_t percentile) const;
        core::Money Equity(std::int32_t percentile) const;
        double GrowthRate(std::int32_t percentile) const;
        core::Money MeanEquity() const;
        double MeanGrowthRate() const;
        double WorstDrawdown() const;
        double AssignmentFrequency() const;

    private:
        explicit OutcomeDistribution(std::vector<PathOutcome>&& ordered);

        std::vector<PathOutcome> ordered;
    };
}
