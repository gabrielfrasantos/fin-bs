#pragma once

#include "source/core/Money.hpp"
#include <span>
#include <vector>

namespace fin::estimation
{
    std::vector<double> LogPrices(std::span<const core::Money> prices);
    std::vector<double> LogReturns(std::span<const core::Money> prices);
}
