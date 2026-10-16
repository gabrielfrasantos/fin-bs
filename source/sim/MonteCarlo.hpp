#pragma once

#include "source/core/Date.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include "source/sim/Rng.hpp"
#include "source/sim/WheelScript.hpp"
#include "source/strategy/Projection.hpp"
#include <cstdint>
#include <vector>

namespace fin::sim
{
    class MonteCarlo
    {
    public:
        MonteCarlo(const strategy::Projection& wheel, const WheelScript& script, const GbmPathGenerator& generator);

        OutcomeDistribution Over(std::int32_t paths, Rng& rng) const;

    private:
        PathOutcome Once(Rng& rng) const;

        strategy::Projection wheel;
        WheelScript script;
        GbmPathGenerator generator;
        std::vector<core::Date> schedule;
    };
}
