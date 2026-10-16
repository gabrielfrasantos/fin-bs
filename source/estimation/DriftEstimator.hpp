#pragma once

#include "source/core/Rate.hpp"
#include "source/estimation/LineFit.hpp"
#include "source/estimation/PriceSeries.hpp"
#include <optional>

namespace fin::estimation
{
    class DriftEstimator
    {
    public:
        explicit DriftEstimator(const LineFit& fit);

        std::optional<core::Rate> LogDrift(const PriceSeries& series) const;

    private:
        const LineFit& fit;
    };
}
