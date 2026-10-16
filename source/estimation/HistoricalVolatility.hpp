#pragma once

#include "source/core/Volatility.hpp"
#include <span>

namespace fin::estimation
{
    class HistoricalVolatility
    {
    public:
        static core::Volatility From(std::span<const double> logReturns, double periodsPerYear);
    };
}
