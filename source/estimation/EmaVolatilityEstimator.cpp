#include "source/estimation/EmaVolatilityEstimator.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "numerical/filters/passive/ExponentialMovingAverage.hpp"
#include <cmath>

namespace fin::estimation
{
    EmaVolatilityEstimator EmaVolatilityEstimator::WithSmoothingFactor(double smoothingFactor, double periodsPerYear)
    {
        return EmaVolatilityEstimator(smoothingFactor, periodsPerYear);
    }

    EmaVolatilityEstimator EmaVolatilityEstimator::WithHalfLife(double halfLifeInPeriods, double periodsPerYear)
    {
        really_assert(halfLifeInPeriods > 0.0);

        return EmaVolatilityEstimator(1.0 - std::exp2(-1.0 / halfLifeInPeriods), periodsPerYear);
    }

    double EmaVolatilityEstimator::SmoothingFactor() const
    {
        return smoothingFactor;
    }

    core::Volatility EmaVolatilityEstimator::From(std::span<const double> logReturns) const
    {
        really_assert(!logReturns.empty());

        filters::passive::ExponentialMovingAverage<double> smoother(smoothingFactor, logReturns.front() * logReturns.front());

        auto variance = logReturns.front() * logReturns.front();
        for (auto logReturn : logReturns.subspan(1))
            variance = smoother.Filter(logReturn * logReturn);

        return core::Volatility::FromPeriodDeviation(std::sqrt(variance), periodsPerYear);
    }

    std::vector<core::Volatility> EmaVolatilityEstimator::Track(std::span<const double> logReturns) const
    {
        really_assert(!logReturns.empty());

        filters::passive::ExponentialMovingAverage<double> smoother(smoothingFactor, logReturns.front() * logReturns.front());

        std::vector<core::Volatility> result;
        result.reserve(logReturns.size());
        result.push_back(core::Volatility::FromPeriodDeviation(std::abs(logReturns.front()), periodsPerYear));

        for (auto logReturn : logReturns.subspan(1))
            result.push_back(core::Volatility::FromPeriodDeviation(std::sqrt(smoother.Filter(logReturn * logReturn)), periodsPerYear));

        return result;
    }

    EmaVolatilityEstimator::EmaVolatilityEstimator(double smoothingFactor, double periodsPerYear)
        : smoothingFactor(smoothingFactor)
        , periodsPerYear(periodsPerYear)
    {
        really_assert(smoothingFactor > 0.0 && smoothingFactor <= 1.0);
        really_assert(periodsPerYear > 0.0);
    }
}
