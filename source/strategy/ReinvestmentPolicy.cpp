#include "source/strategy/ReinvestmentPolicy.hpp"
#include "infra/util/ReallyAssert.hpp"

namespace fin::strategy
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;
    }

    ReinvestmentPolicy ReinvestmentPolicy::WithCashBuffer(std::int32_t bufferBasisPoints)
    {
        really_assert(bufferBasisPoints >= 0 && bufferBasisPoints <= basisPointsPerUnit);
        return ReinvestmentPolicy(bufferBasisPoints);
    }

    ReinvestmentPolicy::ReinvestmentPolicy(std::int32_t bufferBasisPoints)
        : bufferBasisPoints(bufferBasisPoints)
    {}

    std::int32_t ReinvestmentPolicy::BufferBasisPoints() const
    {
        return bufferBasisPoints;
    }

    core::Money ReinvestmentPolicy::Retained(const core::Money& freeCash) const
    {
        really_assert(freeCash.Cents() >= 0);
        return (freeCash * bufferBasisPoints) / basisPointsPerUnit;
    }

    core::Money ReinvestmentPolicy::Investable(const core::Money& freeCash) const
    {
        return freeCash - Retained(freeCash);
    }

    std::int64_t ReinvestmentPolicy::ContractsFor(const core::Money& freeCash, const core::Money& collateralPerContract) const
    {
        really_assert(collateralPerContract.GetCurrency() == freeCash.GetCurrency());
        really_assert(collateralPerContract.Cents() > 0);

        if (freeCash.Cents() <= 0)
            return 0;

        return Investable(freeCash).Cents() / collateralPerContract.Cents();
    }
}
