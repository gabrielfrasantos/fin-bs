#include "source/pricing/NormalDistribution.hpp"
#include "numerical/math/Math.hpp"
#include <cmath>
#include <numbers>

namespace fin::pricing
{
    double StandardNormalCdf(double x)
    {
        return 0.5 * math::Erfc(-x / std::numbers::sqrt2);
    }

    double StandardNormalPdf(double x)
    {
        constexpr double oneOverSqrtTwoPi = std::numbers::inv_sqrtpi / std::numbers::sqrt2;

        return oneOverSqrtTwoPi * std::exp(-0.5 * x * x);
    }
}
