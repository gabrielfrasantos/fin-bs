#include "source/estimation/HistoricalVolatility.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <cmath>

namespace fin::estimation
{
    core::Volatility HistoricalVolatility::From(std::span<const double> logReturns, double periodsPerYear)
    {
        really_assert(logReturns.size() >= 2);
        really_assert(periodsPerYear > 0.0);

        double mean = 0.0;
        for (auto logReturn : logReturns)
            mean += logReturn;
        mean /= static_cast<double>(logReturns.size());

        double sumOfSquares = 0.0;
        for (auto logReturn : logReturns)
            sumOfSquares += (logReturn - mean) * (logReturn - mean);

        const auto variance = sumOfSquares / static_cast<double>(logReturns.size() - 1);

        return core::Volatility::FromPeriodDeviation(std::sqrt(variance), periodsPerYear);
    }
}
