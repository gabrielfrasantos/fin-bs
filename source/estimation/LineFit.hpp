#pragma once

#include <cstddef>
#include <optional>
#include <span>

namespace fin::estimation
{
    class LineFit
    {
    protected:
        LineFit() = default;
        LineFit(const LineFit& other) = delete;
        LineFit& operator=(const LineFit& other) = delete;
        ~LineFit() = default;

    public:
        virtual std::size_t MaximumObservations() const = 0;

        std::optional<double> Slope(std::span<const double> abscissa, std::span<const double> ordinate) const;

    protected:
        virtual std::optional<double> StandardisedSlope(std::span<const double> abscissa, std::span<const double> ordinate) const = 0;
    };
}
