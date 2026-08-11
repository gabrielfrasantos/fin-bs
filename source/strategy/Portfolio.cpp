#include "source/strategy/Portfolio.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

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

    std::int64_t Portfolio::Shares() const
    {
        std::int64_t held = 0;

        for (const auto& lot : lots)
            held += lot.quantity;

        return held;
    }

    std::int64_t Portfolio::CoveredShares() const
    {
        std::int64_t covered = 0;

        for (const auto& position : shorts)
            if (position.contract.Type() == core::OptionType::Call)
                covered += SharesUnder(position);

        return covered;
    }

    std::int64_t Portfolio::UncoveredShares() const
    {
        return Shares() - CoveredShares();
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
        really_assert(FreeCash() >= amount);

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
        really_assert(UncoveredShares() >= static_cast<std::int64_t>(contract.Multiplier()) * contracts);

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
            lots.push_back({ quantity, contract.Strike() });
        }
        else
        {
            Deliver(quantity);
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

    void Portfolio::Deliver(std::int64_t quantity)
    {
        really_assert(Shares() >= quantity);

        while (quantity > 0)
        {
            auto& lot = lots.front();
            const auto delivered = std::min(quantity, lot.quantity);

            lot.quantity -= delivered;
            quantity -= delivered;

            if (lot.quantity == 0)
                lots.erase(lots.begin());
        }
    }

    core::Money Equity(const Portfolio& portfolio, const core::Money& spot)
    {
        really_assert(spot.GetCurrency() == portfolio.GetCurrency());
        really_assert(spot.Cents() >= 0);

        auto equity = portfolio.Cash() + spot * portfolio.Shares();

        for (const auto& position : portfolio.ShortPositions())
            equity -= IntrinsicLiability(position, spot);

        return equity;
    }
}
