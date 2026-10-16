#include "source/pricing/ImpliedVolatility.hpp"
#include "source/pricing/BlackScholes.hpp"
#include "source/pricing/Greeks.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

namespace fin::pricing
{
    namespace
    {
        constexpr double maxVolatility = 10.0;
        constexpr double tolerance = 1e-12;
        constexpr uint32_t maxIterations = 128;

        double InitialGuess(const OptionParameters& parameters, double observedPrice)
        {
            return std::sqrt(2.0 * std::numbers::pi / parameters.timeToExpiry) * observedPrice / parameters.spot.ToDouble();
        }
    }

    std::optional<core::Volatility> ImpliedVolatility::From(const OptionParameters& parameters, double observedPrice)
    {
        if (!std::isfinite(observedPrice) || parameters.timeToExpiry <= 0.0)
            return {};

        const double intrinsic = BlackScholes(parameters, core::Volatility::Zero()).Price();
        const double saturated = BlackScholes(parameters, core::Volatility::FromAnnualized(maxVolatility)).Price();

        if (observedPrice < intrinsic)
            return {};

        if (observedPrice == intrinsic)
            return core::Volatility::Zero();

        if (observedPrice > saturated)
            return {};

        double low = 0.0;
        double high = maxVolatility;
        double candidate = std::clamp(InitialGuess(parameters, observedPrice), 1e-4, 1.0);
        const double priceTolerance = 16.0 * std::numeric_limits<double>::epsilon() * parameters.spot.ToDouble();

        for (uint32_t iteration = 0; iteration != maxIterations; ++iteration)
        {
            const auto volatility = core::Volatility::FromAnnualized(candidate);
            const double difference = BlackScholes(parameters, volatility).Price() - observedPrice;

            if (std::abs(difference) <= priceTolerance)
                return volatility;

            if (difference > 0.0)
                high = candidate;
            else
                low = candidate;

            const double vega = Greeks(parameters, volatility).Vega();
            double next = vega > 0.0 ? candidate - difference / vega : 0.5 * (low + high);

            if (next <= low || next >= high)
                next = 0.5 * (low + high);

            if (std::abs(next - candidate) <= tolerance || high - low <= tolerance)
                return core::Volatility::FromAnnualized(next);

            candidate = next;
        }

        return {};
    }
}
