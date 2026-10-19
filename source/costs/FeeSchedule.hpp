#pragma once

#include "source/core/Money.hpp"
#include <cstdint>

namespace fin::costs
{
    struct FeeSchedule
    {
        core::Money optionFeePerContract;
        core::Money shareCommissionPerTrade;
        core::Money handlingFeePerTrade;
        core::Money connectivityFeeCapPerExchangePerYear;
        std::int32_t connectivityBasisPoints;
        std::int32_t currencyHandlingBasisPoints;
        std::int32_t debitInterestBasisPoints;
    };

    bool IsWellFormed(const FeeSchedule& schedule);
    core::Currency CurrencyOf(const FeeSchedule& schedule);
}
