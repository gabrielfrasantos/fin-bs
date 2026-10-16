#include "source/strategy/CompoundingLedger.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::strategy
{
    CompoundingLedger::CompoundingLedger(core::Currency currency)
        : currency(currency)
    {}

    void CompoundingLedger::Post(const core::Date& on, CashflowKind kind, const core::Money& amount)
    {
        really_assert(amount.GetCurrency() == currency);
        really_assert(HasPermittedSign(kind, amount));

        const auto after = std::upper_bound(entries.begin(), entries.end(), on, [](const core::Date& posted, const Cashflow& entry)
            {
                return posted < entry.on;
            });

        entries.insert(after, { on, kind, amount });
    }

    std::span<const Cashflow> CompoundingLedger::Entries() const
    {
        return entries;
    }

    core::Money CompoundingLedger::Net() const
    {
        auto net = core::Money::Zero(currency);

        for (const auto& entry : entries)
            net += entry.amount;

        return net;
    }

    core::Money CompoundingLedger::NetOf(CashflowKind kind) const
    {
        auto net = core::Money::Zero(currency);

        for (const auto& entry : entries)
            if (entry.kind == kind)
                net += entry.amount;

        return net;
    }

    bool HasPermittedSign(CashflowKind kind, const core::Money& amount)
    {
        switch (kind)
        {
            case CashflowKind::Fee:
            case CashflowKind::Tax:
            case CashflowKind::Fx:
                return amount.Cents() <= 0;
            case CashflowKind::Dividend:
                return amount.Cents() >= 0;
            default:
                return true;
        }
    }
}
