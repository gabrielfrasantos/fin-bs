#pragma once

#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace fin::strategy
{
    enum class CashflowKind : std::uint8_t
    {
        Premium,
        Fee,
        Assignment,
        Dividend,
        Tax,
        Fx
    };

    struct Cashflow
    {
        core::Date on;
        CashflowKind kind;
        core::Money amount;
    };

    class CompoundingLedger
    {
    public:
        explicit CompoundingLedger(core::Currency currency);

        void Post(const core::Date& on, CashflowKind kind, const core::Money& amount);

        std::span<const Cashflow> Entries() const;
        core::Money Net() const;
        core::Money NetOf(CashflowKind kind) const;

    private:
        core::Currency currency;
        std::vector<Cashflow> entries;
    };

    bool HasPermittedSign(CashflowKind kind, const core::Money& amount);
}
