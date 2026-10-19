#pragma once

#include "source/core/Money.hpp"
#include <cstdint>

namespace fin::strategy
{
    class ReinvestmentPolicy
    {
    public:
        static ReinvestmentPolicy WithCashBuffer(std::int32_t bufferBasisPoints);
        static ReinvestmentPolicy WithCashBufferAndCap(std::int32_t bufferBasisPoints, std::int32_t capBasisPoints);

        std::int32_t BufferBasisPoints() const;
        std::int32_t CapBasisPoints() const;
        core::Money Retained(const core::Money& freeCash) const;
        core::Money Investable(const core::Money& freeCash) const;
        std::int64_t ContractsFor(const core::Money& freeCash, const core::Money& collateralPerContract) const;
        core::Money Ceiling(const core::Money& openingEquity) const;
        std::int64_t ContractsFor(const core::Money& freeCash, const core::Money& collateralPerContract, const core::Money& openingEquity) const;

    private:
        ReinvestmentPolicy(std::int32_t bufferBasisPoints, std::int32_t capBasisPoints);

        std::int32_t bufferBasisPoints;
        std::int32_t capBasisPoints;
    };
}
