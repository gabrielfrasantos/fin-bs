#pragma once

#include "source/optimize/Recommendation.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include "source/strategy/Book.hpp"
#include "source/strategy/CompoundingLedger.hpp"
#include <span>
#include <string>
#include <vector>

namespace fin::present
{
    struct ResultTable
    {
        std::vector<std::string> headings;
        std::vector<std::vector<std::string>> rows;

        bool operator==(const ResultTable& other) const = default;
    };

    ResultTable CycleTable(std::span<const strategy::BookRow> rows);
    ResultTable LedgerTable(const strategy::CompoundingLedger& ledger);
    ResultTable OutcomeTable(const sim::OutcomeDistribution& distribution);
    ResultTable RecommendationTable(const optimize::Recommendation& recommendation);
}
