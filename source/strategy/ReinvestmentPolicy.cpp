#include "source/strategy/ReinvestmentPolicy.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::strategy
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;
    }

    ReinvestmentPolicy ReinvestmentPolicy::WithCashBuffer(std::int32_t bufferBasisPoints)
    {
        return WithCashBufferAndCap(bufferBasisPoints, basisPointsPerUnit);
    }

    ReinvestmentPolicy ReinvestmentPolicy::WithCashBufferAndCap(std::int32_t bufferBasisPoints, std::int32_t capBasisPoints)
    {
        really_assert(bufferBasisPoints >= 0 && bufferBasisPoints <= basisPointsPerUnit);
        really_assert(capBasisPoints >= 1 && capBasisPoints <= basisPointsPerUnit);
        return ReinvestmentPolicy(bufferBasisPoints, capBasisPoints);
    }

    ReinvestmentPolicy::ReinvestmentPolicy(std::int32_t bufferBasisPoints, std::int32_t capBasisPoints)
        : bufferBasisPoints(bufferBasisPoints)
        , capBasisPoints(capBasisPoints)
    {}

    std::int32_t ReinvestmentPolicy::BufferBasisPoints() const
    {
        return bufferBasisPoints;
    }

    std::int32_t ReinvestmentPolicy::CapBasisPoints() const
    {
        return capBasisPoints;
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

    core::Money ReinvestmentPolicy::Ceiling(const core::Money& openingEquity) const
    {
        if (openingEquity.Cents() <= 0)
            return core::Money::Zero(openingEquity.GetCurrency());

        return (openingEquity * capBasisPoints) / basisPointsPerUnit;
    }

    std::int64_t ReinvestmentPolicy::ContractsFor(const core::Money& freeCash, const core::Money& collateralPerContract, const core::Money& openingEquity) const
    {
        really_assert(openingEquity.GetCurrency() == freeCash.GetCurrency());

        const auto affordable = ContractsFor(freeCash, collateralPerContract);
        const auto permitted = Ceiling(openingEquity).Cents() / collateralPerContract.Cents();

        return std::min(affordable, permitted);
    }
}
