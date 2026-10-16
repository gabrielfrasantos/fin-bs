#include "source/tax/Box3Parameters.hpp"
#include "infra/util/ReallyAssert.hpp"

namespace fin::tax
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;

        bool IsFraction(std::int32_t basisPoints)
        {
            return basisPoints >= 0 && basisPoints <= basisPointsPerUnit;
        }
    }

    bool IsWellFormed(const Box3Parameters& parameters)
    {
        return parameters.taxYear > 0 &&
               parameters.taxFreeAllowance.Cents() >= 0 &&
               IsFraction(parameters.depositReturnBasisPoints) &&
               IsFraction(parameters.investmentReturnBasisPoints) &&
               IsFraction(parameters.taxBasisPoints) &&
               IsFraction(parameters.dividendWithholdingBasisPoints);
    }

    core::Currency CurrencyOf(const Box3Parameters& parameters)
    {
        really_assert(IsWellFormed(parameters));
        return parameters.taxFreeAllowance.GetCurrency();
    }

    core::Money AtRate(const core::Money& amount, std::int32_t basisPoints)
    {
        really_assert(IsFraction(basisPoints));

        return (amount * static_cast<std::int64_t>(basisPoints)) / static_cast<std::int64_t>(basisPointsPerUnit);
    }
}
