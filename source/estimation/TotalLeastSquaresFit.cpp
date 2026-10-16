#include "source/estimation/TotalLeastSquaresFit.hpp"
#include "numerical/estimators/offline/TotalLeastSquares.hpp"

namespace fin::estimation
{
    namespace
    {
        using Observations = math::Vector<double, TotalLeastSquaresFit::maximumObservations>;

        Observations PaddedWithTheCentroid(std::span<const double> standardised)
        {
            Observations result{};

            for (std::size_t i = 0; i != standardised.size(); ++i)
                result.at(i, 0) = standardised[i];

            return result;
        }
    }

    std::size_t TotalLeastSquaresFit::MaximumObservations() const
    {
        return maximumObservations;
    }

    std::optional<double> TotalLeastSquaresFit::StandardisedSlope(std::span<const double> abscissa, std::span<const double> ordinate) const
    {
        estimators::TotalLeastSquares<double, maximumObservations, 1> regression;

        if (!regression.Fit(PaddedWithTheCentroid(abscissa), PaddedWithTheCentroid(ordinate)))
            return std::nullopt;

        return regression.Coefficients().at(0, 0);
    }
}
