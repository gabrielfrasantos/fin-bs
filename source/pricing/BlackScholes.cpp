#include "source/pricing/BlackScholes.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/pricing/NormalDistribution.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace fin::pricing
{
    BlackScholes::BlackScholes(const OptionParameters& parameters, const core::Volatility& volatility)
        : spot(parameters.spot.ToDouble())
        , strike(parameters.strike.ToDouble())
        , spotDiscountFactor(parameters.dividendYield.DiscountFactor(parameters.timeToExpiry))
        , strikeDiscountFactor(parameters.riskFreeRate.DiscountFactor(parameters.timeToExpiry))
        , deviation(volatility.OverHorizon(parameters.timeToExpiry))
        , type(parameters.type)
    {
        really_assert(parameters.spot.GetCurrency() == parameters.strike.GetCurrency());
        really_assert(parameters.spot.Cents() > 0);
        really_assert(parameters.strike.Cents() > 0);
        really_assert(parameters.timeToExpiry >= 0.0);
        really_assert(parameters.dividendYield >= core::Rate::Zero());

        const double logForwardMoneyness = std::log(spot * spotDiscountFactor / (strike * strikeDiscountFactor));

        if (deviation > 0.0)
        {
            d1 = logForwardMoneyness / deviation + 0.5 * deviation;
            d2 = d1 - deviation;
        }
        else
        {
            d1 = logForwardMoneyness == 0.0 ? 0.0 : std::copysign(std::numeric_limits<double>::infinity(), logForwardMoneyness);
            d2 = d1;
        }
    }

    double BlackScholes::D1() const
    {
        return d1;
    }

    double BlackScholes::D2() const
    {
        return d2;
    }

    double BlackScholes::Deviation() const
    {
        return deviation;
    }

    double BlackScholes::SpotDiscountFactor() const
    {
        return spotDiscountFactor;
    }

    double BlackScholes::StrikeDiscountFactor() const
    {
        return strikeDiscountFactor;
    }

    double BlackScholes::Price() const
    {
        if (type == core::OptionType::Call)
            return spot * spotDiscountFactor * StandardNormalCdf(d1) - strike * strikeDiscountFactor * StandardNormalCdf(d2);

        return strike * strikeDiscountFactor * StandardNormalCdf(-d2) - spot * spotDiscountFactor * StandardNormalCdf(-d1);
    }

    double BlackScholes::IntrinsicValue() const
    {
        if (type == core::OptionType::Call)
            return std::max(spot - strike, 0.0);

        return std::max(strike - spot, 0.0);
    }

    double BlackScholes::ExtrinsicValue() const
    {
        return Price() - IntrinsicValue();
    }
}
