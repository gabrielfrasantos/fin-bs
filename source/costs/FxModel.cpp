#include "source/costs/FxModel.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <cmath>

namespace fin::costs
{
    namespace
    {
        constexpr core::Currency baseCurrency = core::Currency::Eur;
        constexpr std::int64_t basisPointsPerUnit = 10000;
    }

    FxModel FxModel::From(core::Currency foreign, double foreignPerBase, std::int32_t handlingBasisPoints)
    {
        return FxModel(foreign, foreignPerBase, handlingBasisPoints);
    }

    core::Currency FxModel::Base() const
    {
        return baseCurrency;
    }

    core::Currency FxModel::Foreign() const
    {
        return foreign;
    }

    core::Money FxModel::AtSpot(const core::Money& amount) const
    {
        if (amount.GetCurrency() == baseCurrency)
            return core::Money::FromAmount(amount.ToDouble() * foreignPerBase, foreign);

        return core::Money::FromAmount(amount.ToDouble() / foreignPerBase, baseCurrency);
    }

    core::Money FxModel::HandlingCost(const core::Money& amount) const
    {
        const auto magnitude = amount.Cents() < 0 ? -amount : amount;

        return (magnitude * static_cast<std::int64_t>(handlingBasisPoints)) / basisPointsPerUnit;
    }

    core::Money FxModel::Convert(const core::Money& amount) const
    {
        const auto converted = AtSpot(amount);

        return converted - HandlingCost(converted);
    }

    FxModel::FxModel(core::Currency foreign, double foreignPerBase, std::int32_t handlingBasisPoints)
        : foreign(foreign)
        , foreignPerBase(foreignPerBase)
        , handlingBasisPoints(handlingBasisPoints)
    {
        really_assert(foreign != baseCurrency);
        really_assert(std::isfinite(foreignPerBase) && foreignPerBase > 0.0);
        really_assert(handlingBasisPoints >= 0 && handlingBasisPoints < basisPointsPerUnit);
    }
}
