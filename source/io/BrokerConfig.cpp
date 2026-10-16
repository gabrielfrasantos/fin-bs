#include "source/io/BrokerConfig.hpp"
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
    }

    BrokerConfig::BrokerConfig(const std::vector<std::string>& document)
    {
        try
        {
            infra::JsonFileReader reader(document);
            const auto& navigator = reader.GetNavigator();

            const auto currency = CurrencyNamed(navigator / infra::JsonStringNavigatorToken{ "currency" });

            schedule = costs::FeeSchedule{
                .optionFeePerContract = AmountAt(navigator, "option_fee_per_contract_cents", currency),
                .shareCommissionPerTrade = AmountAt(navigator, "share_commission_per_trade_cents", currency),
                .handlingFeePerTrade = AmountAt(navigator, "handling_fee_per_trade_cents", currency),
                .connectivityFeeCapPerExchangePerYear = AmountAt(navigator, "connectivity_fee_cap_per_exchange_per_year_cents", currency),
                .connectivityBasisPoints = BasisPointsAt(navigator, "connectivity_basis_points"),
                .currencyHandlingBasisPoints = BasisPointsAt(navigator, "currency_handling_basis_points")
            };
        }
        catch (const std::exception& error)
        {
            schedule.reset();
            failure = error.what();
        }
    }

    const std::optional<costs::FeeSchedule>& BrokerConfig::Schedule() const
    {
        return schedule;
    }

    const std::string& BrokerConfig::Failure() const
    {
        return failure;
    }
}
