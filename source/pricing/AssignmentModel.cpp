#include "source/pricing/AssignmentModel.hpp"
#include "infra/util/ReallyAssert.hpp"
#include "source/pricing/BlackScholes.hpp"

namespace fin::pricing
{
    AssignmentOutcome AssignmentModel::AtExpiry(const core::OptionContract& contract, const core::Money& spot)
    {
        if (contract.Type() == core::OptionType::Call)
            return spot > contract.Strike() ? AssignmentOutcome::Assigned : AssignmentOutcome::NotAssigned;

        return spot < contract.Strike() ? AssignmentOutcome::Assigned : AssignmentOutcome::NotAssigned;
    }

    AssignmentOutcome AssignmentModel::BeforeExDividend(const OptionParameters& parameters, const core::Volatility& volatility, const core::Money& dividendPerShare)
    {
        really_assert(parameters.type == core::OptionType::Call);
        really_assert(parameters.spot.GetCurrency() == dividendPerShare.GetCurrency());
        really_assert(dividendPerShare.Cents() >= 0);

        const BlackScholes model(parameters, volatility);

        if (model.IntrinsicValue() <= 0.0)
            return AssignmentOutcome::NotAssigned;

        return dividendPerShare.ToDouble() > model.ExtrinsicValue() ? AssignmentOutcome::Assigned : AssignmentOutcome::NotAssigned;
    }
}
