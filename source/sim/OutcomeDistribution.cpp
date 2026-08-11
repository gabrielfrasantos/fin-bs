#include "source/sim/OutcomeDistribution.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <numeric>
#include <utility>

namespace fin::sim
{
    namespace
    {
        constexpr std::int32_t wholeDistribution = 100;
    }

    OutcomeDistribution OutcomeDistribution::Of(std::vector<PathOutcome> outcomes)
    {
        really_assert(!outcomes.empty());

        for (const auto& outcome : outcomes)
        {
            really_assert(outcome.positions > 0);
            really_assert(outcome.assignments >= 0 && outcome.assignments <= outcome.positions);
            really_assert(outcome.terminalEquity.GetCurrency() == outcomes.front().terminalEquity.GetCurrency());
        }

        std::sort(outcomes.begin(), outcomes.end(), [](const PathOutcome& lhs, const PathOutcome& rhs)
            {
                return lhs.terminalEquity < rhs.terminalEquity;
            });

        return OutcomeDistribution(std::move(outcomes));
    }

    OutcomeDistribution::OutcomeDistribution(std::vector<PathOutcome>&& ordered)
        : ordered(std::move(ordered))
    {}

    std::size_t OutcomeDistribution::Paths() const
    {
        return ordered.size();
    }

    const PathOutcome& OutcomeDistribution::At(std::int32_t percentile) const
    {
        really_assert(percentile > 0 && percentile <= wholeDistribution);

        const auto paths = static_cast<std::int64_t>(ordered.size());
        const auto rank = (percentile * paths + wholeDistribution - 1) / wholeDistribution;

        return ordered[static_cast<std::size_t>(rank - 1)];
    }

    core::Money OutcomeDistribution::Equity(std::int32_t percentile) const
    {
        return At(percentile).terminalEquity;
    }

    double OutcomeDistribution::GrowthRate(std::int32_t percentile) const
    {
        return At(percentile).growthRate;
    }

    core::Money OutcomeDistribution::MeanEquity() const
    {
        auto total = core::Money::Zero(ordered.front().terminalEquity.GetCurrency());

        for (const auto& outcome : ordered)
            total += outcome.terminalEquity;

        return total / static_cast<std::int64_t>(ordered.size());
    }

    double OutcomeDistribution::MeanGrowthRate() const
    {
        const auto total = std::accumulate(ordered.begin(), ordered.end(), 0.0, [](double sum, const PathOutcome& outcome)
            {
                return sum + outcome.growthRate;
            });

        return total / static_cast<double>(ordered.size());
    }

    double OutcomeDistribution::WorstDrawdown() const
    {
        return std::max_element(ordered.begin(), ordered.end(), [](const PathOutcome& lhs, const PathOutcome& rhs)
            {
                return lhs.maximumDrawdown < rhs.maximumDrawdown;
            })
            ->maximumDrawdown;
    }

    double OutcomeDistribution::AssignmentFrequency() const
    {
        std::int64_t assignments = 0;
        std::int64_t positions = 0;

        for (const auto& outcome : ordered)
        {
            assignments += outcome.assignments;
            positions += outcome.positions;
        }

        return static_cast<double>(assignments) / static_cast<double>(positions);
    }
}
