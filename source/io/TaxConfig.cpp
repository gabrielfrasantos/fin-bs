#include "source/io/TaxConfig.hpp"
#include "infra/syntax/JsonFileReader.hpp"
#include <exception>
#include <stdexcept>

namespace fin::io
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;

        core::Currency CurrencyNamed(const std::string& name)
        {
            if (name == "EUR")
                return core::Currency::Eur;

            if (name == "USD")
                return core::Currency::Usd;

            throw std::runtime_error("Currency " + name + " is not one this project holds");
        }

        core::Money AmountAt(const infra::JsonObjectNavigator& navigator, const std::string& key, core::Currency currency)
        {
            const auto cents = navigator / infra::JsonIntegerNavigatorToken{ key };

            if (cents < 0)
                throw std::runtime_error("Integer " + key + " is negative");

            return core::Money::FromCents(cents, currency);
        }

        std::int32_t BasisPointsAt(const infra::JsonObjectNavigator& navigator, const std::string& key)
        {
            const auto basisPoints = navigator / infra::JsonIntegerNavigatorToken{ key };

            if (basisPoints < 0 || basisPoints > basisPointsPerUnit)
                throw std::runtime_error("Integer " + key + " is not a fraction of the whole amount");

            return basisPoints;
        }

        std::int32_t YearAt(const infra::JsonObjectNavigator& navigator, const std::string& key)
        {
            const auto year = navigator / infra::JsonIntegerNavigatorToken{ key };

            if (year <= 0)
                throw std::runtime_error("Integer " + key + " is not a year of the common era");

            return year;
        }
    }

    TaxConfig::TaxConfig(const std::vector<std::string>& document)
    {
        try
        {
            infra::JsonFileReader reader(document);
            const auto& navigator = reader.GetNavigator();

            const auto currency = CurrencyNamed(navigator / infra::JsonStringNavigatorToken{ "currency" });

            parameters = tax::Box3Parameters{
                .taxYear = YearAt(navigator, "tax_year"),
                .taxFreeAllowance = AmountAt(navigator, "tax_free_allowance_cents", currency),
                .depositReturnBasisPoints = BasisPointsAt(navigator, "deposit_return_basis_points"),
                .investmentReturnBasisPoints = BasisPointsAt(navigator, "investment_return_basis_points"),
                .taxBasisPoints = BasisPointsAt(navigator, "tax_basis_points"),
                .dividendWithholdingBasisPoints = BasisPointsAt(navigator, "dividend_withholding_basis_points")
            };
        }
        catch (const std::exception& error)
        {
            parameters.reset();
            failure = error.what();
        }
    }

    const std::optional<tax::Box3Parameters>& TaxConfig::Parameters() const
    {
        return parameters;
    }

    const std::string& TaxConfig::Failure() const
    {
        return failure;
    }
}
