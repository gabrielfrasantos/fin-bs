#pragma once

#include "source/core/Money.hpp"
#include <cstdint>

namespace fin::tax
{
    struct Box3Parameters
    {
        std::int32_t taxYear;
        core::Money taxFreeAllowance;
        std::int32_t depositReturnBasisPoints;
        std::int32_t investmentReturnBasisPoints;
        std::int32_t taxBasisPoints;
        std::int32_t dividendWithholdingBasisPoints;
    };

    bool IsWellFormed(const Box3Parameters& parameters);
    core::Currency CurrencyOf(const Box3Parameters& parameters);
    core::Money AtRate(const core::Money& amount, std::int32_t basisPoints);
}
