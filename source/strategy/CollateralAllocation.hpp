#pragma once

#include "source/core/Money.hpp"
#include <cstddef>
#include <span>
#include <vector>

namespace fin::strategy
{
    struct CollateralClaim
    {
        bool fromCash;
        double premiumPerShare;
        core::Money strike;
    };

    double PremiumPerUnitOfCollateral(const CollateralClaim& claim);

    std::vector<std::size_t> ServiceOrder(std::span<const CollateralClaim> claims);
}
