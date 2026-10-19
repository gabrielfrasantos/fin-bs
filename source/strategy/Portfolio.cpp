#include "source/strategy/Portfolio.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>
#include <array>
#include <iterator>

namespace fin::strategy
{
    namespace
    {
        std::int64_t SharesUnder(const ShortOption& position)
        {
            return static_cast<std::int64_t>(position.contract.Multiplier()) * position.contracts;
        }

        core::Money IntrinsicLiability(const ShortOption& position, const core::Money& spot)
        {
            const auto& strike = position.contract.Strike();
            const auto perShare = position.contract.Type() == core::OptionType::Call ? spot - strike : strike - spot;

            if (perShare.Cents() <= 0)
                return core::Money::Zero(spot.GetCurrency());

            return perShare * SharesUnder(position);
        }

        template<class Positions>
        auto FindPosition(Positions& positions, const core::OptionContract& contract)
        {
            return std::find_if(positions.begin(), positions.end(), [&contract](const ShortOption& open)
                {
                    return open.contract == contract;
                });
        }

        const core::Money& SpotOf(std::span<const SpotPrice> prices, const std::string& underlying)
        {
            const auto price = std::find_if(prices.begin(), prices.end(), [&underlying](const SpotPrice& quoted)
                {
                    return quoted.underlying == underlying;
                });

            really_assert(price != prices.end());

            return price->spot;
        }

        bool NamesEachUnderlyingOnce(std::span<const SpotPrice> prices)
        {
            return std::all_of(prices.begin(), prices.end(), [prices](const SpotPrice& price)
                {
                    return std::count_if(prices.begin(), prices.end(), [&price](const SpotPrice& other)
                               {
                                   return other.underlying == price.underlying;
                               }) == 1;
                });
        }
    }

    Portfolio Portfolio::Funded(const core::Money& cash)
    {
        really_assert(cash.Cents() >= 0);
        return Portfolio(cash);
    }

    Portfolio::Portfolio(const core::Money& cash)
        : cash(cash)
    {}

    core::Currency Portfolio::GetCurrency() const
    {
        return cash.GetCurrency();
    }

    const core::Money& Portfolio::Cash() const
    {
        return cash;
    }

    core::Money Portfolio::Collateral() const
    {
        auto reserved = core::Money::Zero(GetCurrency());

        for (const auto& position : shorts)
            if (position.contract.Type() == core::OptionType::Put)
                reserved += position.contract.Notional() * position.contracts;

        return reserved;
    }

    core::Money Portfolio::FreeCash() const
    {
        return cash - Collateral();
    }

    bool Portfolio::Overdrawn() const
    {
        return cash.Cents() < 0;
    }

    std::int64_t Portfolio::Shares(const std::string& underlying) const
    {
        std::int64_t held = 0;

        for (const auto& lot : lots)
            if (lot.underlying == underlying)
                held += lot.quantity;

        return held;
    }

    std::int64_t Portfolio::CoveredShares(const std::string& underlying) const
    {
        std::int64_t covered = 0;

        for (const auto& position : shorts)
            if (position.contract.Type() == core::OptionType::Call && position.contract.Underlying() == underlying)
                covered += SharesUnder(position);

        return covered;
    }

    std::int64_t Portfolio::UncoveredShares(const std::string& underlying) const
    {
        return Shares(underlying) - CoveredShares(underlying);
    }

    std::span<const ShareLot> Portfolio::Lots() const
    {
        return lots;
    }

    std::span<const ShortOption> Portfolio::ShortPositions() const
    {
        return shorts;
    }

    std::int64_t Portfolio::ContractsShortOf(const core::OptionContract& contract) const
    {
        const auto position = FindPosition(shorts, contract);

        return position == shorts.end() ? 0 : position->contracts;
    }

    void Portfolio::Receive(const core::Money& amount)
    {
        really_assert(amount.GetCurrency() == GetCurrency());
        really_assert(amount.Cents() >= 0);

        cash += amount;
    }

    void Portfolio::Pay(const core::Money& amount)
    {
        really_assert(amount.GetCurrency() == GetCurrency());
        really_assert(amount.Cents() >= 0);
        really_assert(Collateral().Cents() == 0 || FreeCash() >= amount);

        cash -= amount;
    }

    void Portfolio::SellPut(const core::OptionContract& contract, std::int64_t contracts)
    {
        really_assert(contract.Type() == core::OptionType::Put);
        really_assert(contract.Strike().GetCurrency() == GetCurrency());
        really_assert(contracts > 0);
        really_assert(FreeCash() >= contract.Notional() * contracts);

        Open(contract, contracts);
    }

    void Portfolio::SellCall(const core::OptionContract& contract, std::int64_t contracts)
    {
        really_assert(contract.Type() == core::OptionType::Call);
        really_assert(contract.Strike().GetCurrency() == GetCurrency());
        really_assert(contracts > 0);
        really_assert(UncoveredShares(contract.Underlying()) >= static_cast<std::int64_t>(contract.Multiplier()) * contracts);

        Open(contract, contracts);
    }

    void Portfolio::Close(const core::OptionContract& contract)
    {
        const auto position = FindPosition(shorts, contract);
        really_assert(position != shorts.end());

        shorts.erase(position);
    }

    void Portfolio::Assign(const core::OptionContract& contract)
    {
        const auto position = FindPosition(shorts, contract);
        really_assert(position != shorts.end());

        const auto quantity = SharesUnder(*position);
        const auto settlement = contract.Notional() * position->contracts;

        shorts.erase(position);

        if (contract.Type() == core::OptionType::Put)
        {
            Pay(settlement);
            lots.push_back({ contract.Underlying(), quantity, contract.Strike() });
        }
        else
        {
            Deliver(contract.Underlying(), quantity);
            Receive(settlement);
        }
    }

    void Portfolio::Open(const core::OptionContract& contract, std::int64_t contracts)
    {
        const auto position = FindPosition(shorts, contract);

        if (position == shorts.end())
            shorts.push_back({ contract, contracts });
        else
            position->contracts += contracts;
    }

    void Portfolio::Deliver(const std::string& underlying, std::int64_t quantity)
    {
        really_assert(Shares(underlying) >= quantity);

        for (auto lot = lots.begin(); lot != lots.end() && quantity > 0;)
        {
            if (lot->underlying != underlying)
            {
                ++lot;
                continue;
            }

            const auto delivered = std::min(quantity, lot->quantity);

            lot->quantity -= delivered;
            quantity -= delivered;

            lot = lot->quantity == 0 ? lots.erase(lot) : std::next(lot);
        }
    }

    core::Money Equity(const Portfolio& portfolio, std::span<const SpotPrice> prices)
    {
        really_assert(NamesEachUnderlyingOnce(prices));

        for (const auto& price : prices)
        {
            really_assert(price.spot.GetCurrency() == portfolio.GetCurrency());
            really_assert(price.spot.Cents() >= 0);
        }

        auto equity = portfolio.Cash();

        for (const auto& lot : portfolio.Lots())
            equity += SpotOf(prices, lot.underlying) * lot.quantity;

        for (const auto& position : portfolio.ShortPositions())
            equity -= IntrinsicLiability(position, SpotOf(prices, position.contract.Underlying()));

        return equity;
    }

    core::Money Equity(const Portfolio& portfolio, const std::string& underlying, const core::Money& spot)
    {
        const std::array quoted{ SpotPrice{ underlying, spot } };

        return Equity(portfolio, quoted);
    }
}
