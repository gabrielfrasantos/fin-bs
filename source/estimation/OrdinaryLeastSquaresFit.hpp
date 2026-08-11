#pragma once

#include "source/estimation/LineFit.hpp"

namespace fin::estimation
{
    class OrdinaryLeastSquaresFit
        : public LineFit
    {
    public:
        static constexpr std::size_t maximumObservations = 8192;

        std::size_t MaximumObservations() const override;

    protected:
        std::optional<double> StandardisedSlope(std::span<const double> abscissa, std::span<const double> ordinate) const override;
    };
}
