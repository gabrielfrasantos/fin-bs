#include "source/pricing/Greeks.hpp"
#include "source/pricing/NormalDistribution.hpp"
#include <cmath>

namespace fin::pricing
{
    Greeks::Greeks(const OptionParameters& parameters, const core::Volatility& volatility)
        : parameters(parameters)
        , volatility(volatility)
        , model(parameters, volatility)
    {}

    core::Delta Greeks::Delta() const
    {
        if (parameters.type == core::OptionType::Call)
            return core::Delta::FromValue(model.SpotDiscountFactor() * StandardNormalCdf(model.D1()));

        return core::Delta::FromValue(-model.SpotDiscountFactor() * StandardNormalCdf(-model.D1()));
    }

    double Greeks::Gamma() const
    {
        if (model.Deviation() <= 0.0)
            return 0.0;

        return model.SpotDiscountFactor() * StandardNormalPdf(model.D1()) / (parameters.spot.ToDouble() * model.Deviation());
    }

    double Greeks::Vega() const
    {
        return parameters.spot.ToDouble() * model.SpotDiscountFactor() * StandardNormalPdf(model.D1()) * std::sqrt(parameters.timeToExpiry);
    }

    double Greeks::Theta() const
    {
        const double spotTerm = parameters.spot.ToDouble() * model.SpotDiscountFactor();
        const double strikeTerm = parameters.strike.ToDouble() * model.StrikeDiscountFactor();
        const double decay = parameters.timeToExpiry > 0.0 ? -spotTerm * StandardNormalPdf(model.D1()) * volatility.Annualized() / (2.0 * std::sqrt(parameters.timeToExpiry)) : 0.0;

        if (parameters.type == core::OptionType::Call)
            return decay - parameters.riskFreeRate.Continuous() * strikeTerm * StandardNormalCdf(model.D2()) + parameters.dividendYield.Continuous() * spotTerm * StandardNormalCdf(model.D1());

        return decay + parameters.riskFreeRate.Continuous() * strikeTerm * StandardNormalCdf(-model.D2()) - parameters.dividendYield.Continuous() * spotTerm * StandardNormalCdf(-model.D1());
    }

    double Greeks::Rho() const
    {
        const double strikeTerm = parameters.strike.ToDouble() * model.StrikeDiscountFactor() * parameters.timeToExpiry;

        if (parameters.type == core::OptionType::Call)
            return strikeTerm * StandardNormalCdf(model.D2());

        return -strikeTerm * StandardNormalCdf(-model.D2());
    }
}
