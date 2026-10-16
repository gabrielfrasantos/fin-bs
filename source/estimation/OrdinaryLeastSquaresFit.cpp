#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "numerical/estimators/offline/LinearRegression.hpp"

namespace fin::estimation
{
    namespace
    {
        using Observations = math::Vector<double, OrdinaryLeastSquaresFit::maximumObservations>;

        Observations PaddedWithTheCentroid(std::span<const double> standardised)
        {
            Observations result{};

            for (std::size_t i = 0; i != standardised.size(); ++i)
                result.at(i, 0) = standardised[i];

            return result;
        }
    }

    std::size_t OrdinaryLeastSquaresFit::MaximumObservations() const
    {
        return maximumObservations;
    }

    std::optional<double> OrdinaryLeastSquaresFit::StandardisedSlope(std::span<const double> abscissa, std::span<const double> ordinate) const
    {
        estimators::LinearRegression<double, maximumObservations, 1> regression;
        regression.Fit(PaddedWithTheCentroid(abscissa), PaddedWithTheCentroid(ordinate));

        return regression.Coefficients().at(1, 0);
    }
}
