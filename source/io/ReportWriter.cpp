#include "source/io/ReportWriter.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <cstdint>

namespace fin::io
{
    namespace
    {
        constexpr std::int64_t centsPerUnit = 100;

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
            }

            really_assert(false);
            return {};
        }
    }

    std::string CycleReport(std::span<const strategy::BookRow> rows)
    {
        std::string report = "underlying,date,state,contracts,premium,fees,assignment,tax,cash,shares,equity\n";

        for (const auto& entry : rows)
            report += entry.underlying + "," +
                      TextOf(entry.row.on) + "," +
                      TextOf(entry.row.state) + "," +
                      std::to_string(entry.row.contracts) + "," +
                      TextOf(entry.row.premium) + "," +
                      TextOf(entry.row.fees) + "," +
                      TextOf(entry.row.assignment) + "," +
                      TextOf(entry.row.tax) + "," +
                      TextOf(entry.row.cash) + "," +
                      std::to_string(entry.row.shares) + "," +
                      TextOf(entry.row.equity) + "\n";

        return report;
    }

    std::string LedgerReport(const strategy::CompoundingLedger& ledger)
    {
        std::string report = "date,kind,amount\n";

        for (const auto& entry : ledger.Entries())
            report += TextOf(entry.on) + "," + TextOf(entry.kind) + "," + TextOf(entry.amount) + "\n";

        return report;
    }
}
