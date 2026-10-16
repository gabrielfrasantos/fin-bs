#include "source/tax/Box3TaxModel.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::tax
{
    core::Money TotalValue(const Box3Assets& assets)
    {
        really_assert(assets.deposits.Cents() >= 0);
        really_assert(assets.investments.Cents() >= 0);

        return assets.deposits + assets.investments;
    }

    Box3TaxModel::Box3TaxModel(const Box3Parameters& parameters)
        : parameters(parameters)
    {
        really_assert(IsWellFormed(parameters));
    }

    std::int32_t Box3TaxModel::Year() const
    {
        return parameters.taxYear;
    }

    core::Money Box3TaxModel::DeemedReturn(const Box3Assets& assets) const
    {
        really_assert(TotalValue(assets).GetCurrency() == CurrencyOf(parameters));

        return AtRate(assets.deposits, parameters.depositReturnBasisPoints) + AtRate(assets.investments, parameters.investmentReturnBasisPoints);
    }

    core::Money Box3TaxModel::TaxableBase(const Box3Assets& assets) const
    {
        really_assert(TotalValue(assets).GetCurrency() == CurrencyOf(parameters));

        return std::max(TotalValue(assets) - parameters.taxFreeAllowance, core::Money::Zero(CurrencyOf(parameters)));
    }

    core::Money Box3TaxModel::TaxableReturn(const Box3Assets& assets) const
    {
        const auto taxable = TaxableBase(assets);

        if (taxable.Cents() == 0)
            return core::Money::Zero(CurrencyOf(parameters));

        return DeemedReturn(assets) * (taxable.ToDouble() / TotalValue(assets).ToDouble());
    }

    core::Money Box3TaxModel::Charge(const Box3Assets& assets) const
    {
        return AtRate(TaxableReturn(assets), parameters.taxBasisPoints);
    }
}
