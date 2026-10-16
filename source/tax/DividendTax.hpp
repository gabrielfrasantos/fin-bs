#pragma once

#include "source/core/Money.hpp"
#include "source/tax/Box3Parameters.hpp"
#include <cstdint>
#include <span>

namespace fin::tax
{
    struct Dividend
    {
        core::Money gross;
        std::int32_t withheldBasisPoints;
    };

    class DividendTax
    {
    public:
        explicit DividendTax(const Box3Parameters& parameters);

        core::Money Withheld(const Dividend& dividend) const;
        core::Money Received(const Dividend& dividend) const;
        core::Money Creditable(const Dividend& dividend) const;
        core::Money Leakage(const Dividend& dividend) const;
        core::Money CreditAgainst(const core::Money& charge, std::span<const Dividend> dividends) const;

    private:
        Box3Parameters parameters;
    };
}
