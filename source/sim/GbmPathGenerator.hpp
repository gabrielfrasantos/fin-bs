#pragma once

#include "source/core/Date.hpp"
#include "source/core/DayCount.hpp"
#include "source/core/Money.hpp"
#include "source/core/Rate.hpp"
#include "source/core/Volatility.hpp"
#include "source/estimation/PriceSeries.hpp"
#include "source/sim/Rng.hpp"
#include <span>

namespace fin::sim
{
    struct GbmParameters
    {
        core::Money initialPrice;
        core::Rate drift;
        core::Volatility volatility;
        core::DayCountConvention convention;
    };

    class GbmPathGenerator
    {
    public:
        explicit GbmPathGenerator(const GbmParameters& parameters);

        core::Rate LogDrift() const;
        estimation::PriceSeries Along(std::span<const core::Date> schedule, Rng& rng) const;

    private:
        GbmParameters parameters;
    };
}
