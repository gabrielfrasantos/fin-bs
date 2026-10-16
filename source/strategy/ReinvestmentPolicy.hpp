#pragma once

#include "source/core/Money.hpp"
#include <cstdint>

namespace fin::strategy
{
    class ReinvestmentPolicy
    {
    public:
        static ReinvestmentPolicy WithCashBuffer(std::int32_t bufferBasisPoints);

        std::int32_t BufferBasisPoints() const;
        core::Money Retained(const core::Money& freeCash) const;
        core::Money Investable(const core::Money& freeCash) const;
        std::int64_t ContractsFor(const core::Money& freeCash, const core::Money& collateralPerContract) const;

    private:
        explicit ReinvestmentPolicy(std::int32_t bufferBasisPoints);

        std::int32_t bufferBasisPoints;
    };
}
