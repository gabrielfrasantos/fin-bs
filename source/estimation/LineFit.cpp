#include "source/estimation/LineFit.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <cmath>
#include <numeric>
#include <vector>

namespace fin::estimation
{
    namespace
    {
        double Centre(std::span<const double> values)
        {
            return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
        }

        double Scale(std::span<const double> values, double centre)
        {
            double sumOfSquares = 0.0;
            for (auto value : values)
                sumOfSquares += (value - centre) * (value - centre);

            return std::sqrt(sumOfSquares);
        }

        std::vector<double> Standardised(std::span<const double> values, double centre, double scale)
        {
            std::vector<double> result;
            result.reserve(values.size());

            for (auto value : values)
                result.push_back((value - centre) / scale);

            return result;
        }
    }

    std::optional<double> LineFit::Slope(std::span<const double> abscissa, std::span<const double> ordinate) const
    {
        really_assert(abscissa.size() == ordinate.size());
        really_assert(abscissa.size() >= 2);
        really_assert(abscissa.size() <= MaximumObservations());

        const auto abscissaCentre = Centre(abscissa);
        const auto ordinateCentre = Centre(ordinate);
        const auto abscissaScale = Scale(abscissa, abscissaCentre);
        const auto ordinateScale = Scale(ordinate, ordinateCentre);

        if (abscissaScale == 0.0)
            return std::nullopt;

        if (ordinateScale == 0.0)
            return 0.0;

        const auto standardisedSlope = StandardisedSlope(Standardised(abscissa, abscissaCentre, abscissaScale),
            Standardised(ordinate, ordinateCentre, ordinateScale));

        if (!standardisedSlope)
            return std::nullopt;

        return *standardisedSlope * ordinateScale / abscissaScale;
    }
}
