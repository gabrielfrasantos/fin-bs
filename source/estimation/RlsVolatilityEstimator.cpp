#include "source/estimation/RlsVolatilityEstimator.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "numerical/estimators/online/RecursiveLeastSquares.hpp"
#include <cmath>

namespace fin::estimation
{
    namespace
    {
        constexpr double diffuseCovariance = 1.0e6;

        using MeanOfSquares = estimators::RecursiveLeastSquares<double, 1>;

        MeanOfSquares::InputMatrix ConstantRegressor()
        {
            MeanOfSquares::InputMatrix regressor{};
            regressor.at(0, 0) = 1.0;

            return regressor;
        }

        double Update(MeanOfSquares& estimator, const MeanOfSquares::InputMatrix& regressor, double logReturn)
        {
            math::Matrix<double, 1, 1> squared{};
            squared.at(0, 0) = logReturn * logReturn;

            estimator.Update(regressor, squared);

            return estimator.Coefficients().at(0, 0);
        }
    }

    RlsVolatilityEstimator RlsVolatilityEstimator::WithForgettingFactor(double forgettingFactor, double periodsPerYear)
    {
        return RlsVolatilityEstimator(forgettingFactor, periodsPerYear);
    }

    double RlsVolatilityEstimator::ForgettingFactor() const
    {
        return forgettingFactor;
    }

    core::Volatility RlsVolatilityEstimator::From(std::span<const double> logReturns) const
    {
        really_assert(!logReturns.empty());

        MeanOfSquares estimator(diffuseCovariance, forgettingFactor);
        const auto regressor = ConstantRegressor();

        double variance = 0.0;
        for (auto logReturn : logReturns)
            variance = Update(estimator, regressor, logReturn);

        return core::Volatility::FromPeriodDeviation(std::sqrt(variance), periodsPerYear);
    }

    std::vector<core::Volatility> RlsVolatilityEstimator::Track(std::span<const double> logReturns) const
    {
        really_assert(!logReturns.empty());

        MeanOfSquares estimator(diffuseCovariance, forgettingFactor);
        const auto regressor = ConstantRegressor();

        std::vector<core::Volatility> result;
        result.reserve(logReturns.size());

        for (auto logReturn : logReturns)
            result.push_back(core::Volatility::FromPeriodDeviation(std::sqrt(Update(estimator, regressor, logReturn)), periodsPerYear));

        return result;
    }

    RlsVolatilityEstimator::RlsVolatilityEstimator(double forgettingFactor, double periodsPerYear)
        : forgettingFactor(forgettingFactor)
        , periodsPerYear(periodsPerYear)
    {
        really_assert(forgettingFactor > 0.0 && forgettingFactor <= 1.0);
        really_assert(periodsPerYear > 0.0);
    }
}
