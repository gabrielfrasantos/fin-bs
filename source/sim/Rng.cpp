#include "source/sim/Rng.hpp"
#include <cmath>
#include <numbers>

namespace fin::sim
{
    namespace
    {
        constexpr double significandScale = 0x1.0p-53;
    }

    Rng::Rng(std::uint64_t seed)
        : engine(seed)
    {}

    double Rng::NextUniform()
    {
        return (static_cast<double>(engine() >> 11) + 0.5) * significandScale;
    }

    double Rng::NextStandardNormal()
    {
        if (spareAvailable)
        {
            spareAvailable = false;
            return spare;
        }

        const auto radius = std::sqrt(-2.0 * std::log(NextUniform()));
        const auto angle = 2.0 * std::numbers::pi * NextUniform();

        spare = radius * std::sin(angle);
        spareAvailable = true;

        return radius * std::cos(angle);
    }
}
