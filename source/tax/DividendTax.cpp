#include "source/tax/DividendTax.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::tax
{
    DividendTax::DividendTax(const Box3Parameters& parameters)
        : parameters(parameters)
    {
        really_assert(IsWellFormed(parameters));
    }

    core::Money DividendTax::Withheld(const Dividend& dividend) const
    {
        really_assert(dividend.gross.GetCurrency() == CurrencyOf(parameters));
        really_assert(dividend.gross.Cents() >= 0);

        return AtRate(dividend.gross, dividend.withheldBasisPoints);
    }

    core::Money DividendTax::Received(const Dividend& dividend) const
    {
        return dividend.gross - Withheld(dividend);
    }

    core::Money DividendTax::Creditable(const Dividend& dividend) const
    {
        return std::min(Withheld(dividend), AtRate(dividend.gross, parameters.dividendWithholdingBasisPoints));
    }

    core::Money DividendTax::Leakage(const Dividend& dividend) const
    {
        return Withheld(dividend) - Creditable(dividend);
    }

    core::Money DividendTax::CreditAgainst(const core::Money& charge, std::span<const Dividend> dividends) const
    {
        really_assert(charge.GetCurrency() == CurrencyOf(parameters));
        really_assert(charge.Cents() >= 0);

        auto credit = core::Money::Zero(CurrencyOf(parameters));

        for (const auto& dividend : dividends)
            credit += Creditable(dividend);

        return std::min(credit, charge);
    }
}
