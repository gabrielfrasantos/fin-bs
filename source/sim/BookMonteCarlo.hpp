#pragma once

#include "source/core/Date.hpp"
#include "source/sim/BookScript.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include "source/sim/OutcomeDistribution.hpp"
#include "source/sim/Rng.hpp"
#include "source/sim/RunObserver.hpp"
#include "source/strategy/Book.hpp"
#include <cstdint>
#include <vector>

namespace fin::sim
{
    class BookMonteCarlo
    {
    public:
        BookMonteCarlo(const strategy::Book& book, const BookScript& script, std::vector<GbmPathGenerator> generators);

        OutcomeDistribution Over(std::int32_t paths, Rng& rng) const;
        OutcomeDistribution Over(std::int32_t paths, Rng& rng, RunObserver& observer) const;

    private:
        PathOutcome Once(Rng& rng) const;

        strategy::Book book;
        BookScript script;
        std::vector<GbmPathGenerator> generators;
        std::vector<core::Date> schedule;
    };
}
