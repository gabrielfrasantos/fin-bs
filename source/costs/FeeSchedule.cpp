#include "source/costs/FeeSchedule.hpp"
#include "infra/util/ReallyAssert.hpp"

namespace fin::costs
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;

        bool SharesCurrencyWith(const core::Money& amount, core::Currency currency)
        {
            return amount.GetCurrency() == currency && amount.Cents() >= 0;
        }

        bool IsFraction(std::int32_t basisPoints)
        {
            return basisPoints >= 0 && basisPoints <= basisPointsPerUnit;
        }
    }

    bool IsWellFormed(const FeeSchedule& schedule)
    {
        const auto currency = schedule.optionFeePerContract.GetCurrency();

        return SharesCurrencyWith(schedule.optionFeePerContract, currency) &&
               SharesCurrencyWith(schedule.shareCommissionPerTrade, currency) &&
               SharesCurrencyWith(schedule.handlingFeePerTrade, currency) &&
               SharesCurrencyWith(schedule.connectivityFeeCapPerExchangePerYear, currency) &&
               IsFraction(schedule.connectivityBasisPoints) &&
               IsFraction(schedule.currencyHandlingBasisPoints) &&
               IsFraction(schedule.debitInterestBasisPoints);
    }

    core::Currency CurrencyOf(const FeeSchedule& schedule)
    {
        really_assert(IsWellFormed(schedule));
        return schedule.optionFeePerContract.GetCurrency();
    }
}
