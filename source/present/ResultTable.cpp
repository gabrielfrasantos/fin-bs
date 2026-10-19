#include "source/present/ResultTable.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <cmath>
#include <cstdint>

namespace fin::present
{
    namespace
    {
        constexpr std::int64_t centsPerUnit = 100;
        constexpr std::int32_t basisPointsPerUnit = 10000;
        constexpr std::int32_t lowerBand = 5;
        constexpr std::int32_t median = 50;
        constexpr std::int32_t upperBand = 95;

        std::string Padded(std::uint32_t part)
        {
            return part < 10 ? "0" + std::to_string(part) : std::to_string(part);
        }

        std::string TextOf(const core::Date& date)
        {
            return std::to_string(date.Year()) + "-" + Padded(date.Month()) + "-" + Padded(date.Day());
        }

        std::string TextOf(const core::Money& money)
        {
            const auto magnitude = money.Cents() < 0 ? -money.Cents() : money.Cents();
            const auto amount = std::to_string(magnitude / centsPerUnit) + "." + Padded(static_cast<std::uint32_t>(magnitude % centsPerUnit));

            return money.Cents() < 0 ? "-" + amount : amount;
        }

        std::string PercentageOf(double fraction)
        {
            const auto basisPoints = static_cast<std::int64_t>(std::llround(fraction * basisPointsPerUnit));
            const auto magnitude = basisPoints < 0 ? -basisPoints : basisPoints;
            const auto percentage = std::to_string(magnitude / centsPerUnit) + "." + Padded(static_cast<std::uint32_t>(magnitude % centsPerUnit)) + "%";

            return basisPoints < 0 ? "-" + percentage : percentage;
        }

        std::string TextOf(strategy::WheelState state)
        {
            switch (state)
            {
                case strategy::WheelState::Cash:
                    return "cash";
                case strategy::WheelState::ShortPut:
                    return "short put";
                case strategy::WheelState::HoldingShares:
                    return "holding shares";
                case strategy::WheelState::ShortCall:
                    return "short call";
            }

            really_assert(false);
            return {};
        }

        std::string TextOf(strategy::CashflowKind kind)
        {
            switch (kind)
            {
                case strategy::CashflowKind::Premium:
                    return "premium";
                case strategy::CashflowKind::Fee:
                    return "fee";
                case strategy::CashflowKind::Assignment:
                    return "assignment";
                case strategy::CashflowKind::Dividend:
                    return "dividend";
                case strategy::CashflowKind::Tax:
                    return "tax";
                case strategy::CashflowKind::Fx:
                    return "conversion";
                case strategy::CashflowKind::Interest:
                    return "interest";
            }

            really_assert(false);
            return {};
        }
    }

    ResultTable CycleTable(std::span<const strategy::BookRow> rows)
    {
        ResultTable table{ { "underlying", "date", "state", "contracts", "premium", "fees", "assignment", "dividend", "tax", "cash", "shares", "equity" }, {} };

        for (const auto& entry : rows)
            table.rows.push_back({ entry.underlying,
                TextOf(entry.row.on),
                TextOf(entry.row.state),
                std::to_string(entry.row.contracts),
                TextOf(entry.row.premium),
                TextOf(entry.row.fees),
                TextOf(entry.row.assignment),
                TextOf(entry.row.dividend),
                TextOf(entry.row.tax),
                TextOf(entry.row.cash),
                std::to_string(entry.row.shares),
                TextOf(entry.row.equity) });

        return table;
    }

    ResultTable LedgerTable(const strategy::CompoundingLedger& ledger)
    {
        ResultTable table{ { "date", "kind", "amount" }, {} };

        for (const auto& entry : ledger.Entries())
            table.rows.push_back({ TextOf(entry.on), TextOf(entry.kind), TextOf(entry.amount) });

        return table;
    }

    ResultTable OutcomeTable(const sim::OutcomeDistribution& distribution)
    {
        ResultTable table{ { "measure", "value" }, {} };

        table.rows.push_back({ "paths", std::to_string(distribution.Paths()) });
        table.rows.push_back({ "equity at the 5th percentile", TextOf(distribution.Equity(lowerBand)) });
        table.rows.push_back({ "equity at the median", TextOf(distribution.Equity(median)) });
        table.rows.push_back({ "equity at the 95th percentile", TextOf(distribution.Equity(upperBand)) });
        table.rows.push_back({ "mean equity", TextOf(distribution.MeanEquity()) });
        table.rows.push_back({ "mean growth a year", PercentageOf(distribution.MeanGrowthRate()) });
        table.rows.push_back({ "worst drawdown", PercentageOf(distribution.WorstDrawdown()) });
        table.rows.push_back({ "assignments per option written", PercentageOf(distribution.AssignmentFrequency()) });
        table.rows.push_back({ "paths that borrowed", PercentageOf(distribution.DebtFrequency()) });

        return table;
    }

    ResultTable RecommendationTable(const optimize::Recommendation& recommendation)
    {
        ResultTable table{ { "lever", "value" }, {} };

        table.rows.push_back({ "target delta", PercentageOf(recommendation.setting.targetDelta.Value()) });
        table.rows.push_back({ "cash buffer", PercentageOf(static_cast<double>(recommendation.setting.cashBufferBasisPoints) / basisPointsPerUnit) });
        table.rows.push_back({ "roll delta limit", PercentageOf(recommendation.setting.rollDeltaLimit.Value()) });
        table.rows.push_back({ "mean growth a year", PercentageOf(recommendation.growthRate) });
        table.rows.push_back({ "equity at the median", TextOf(recommendation.medianEquity) });
        table.rows.push_back({ "worst drawdown", PercentageOf(recommendation.worstDrawdown) });
        table.rows.push_back({ "assignments per option written", PercentageOf(recommendation.assignmentFrequency) });
        table.rows.push_back({ "drawdown limit", recommendation.drawdownBinds ? "breached" : "met" });
        table.rows.push_back({ "assignment limit", recommendation.assignmentBinds ? "breached" : "met" });

        return table;
    }
}
