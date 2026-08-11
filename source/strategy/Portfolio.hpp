#pragma once

#include "source/core/Money.hpp"
#include "source/core/OptionContract.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace fin::strategy
{
    struct ShareLot
    {
        std::int64_t quantity;
        core::Money pricePerShare;
    };

    struct ShortOption
    {
        core::OptionContract contract;
        std::int64_t contracts;
    };

    class Portfolio
    {
    public:
        static Portfolio Funded(const core::Money& cash);

        core::Currency GetCurrency() const;
        const core::Money& Cash() const;
        core::Money Collateral() const;
        core::Money FreeCash() const;

        std::int64_t Shares() const;
        std::int64_t CoveredShares() const;
        std::int64_t UncoveredShares() const;

        std::span<const ShareLot> Lots() const;
        std::span<const ShortOption> ShortPositions() const;
        std::int64_t ContractsShortOf(const core::OptionContract& contract) const;

        void Receive(const core::Money& amount);
        void Pay(const core::Money& amount);

        void SellPut(const core::OptionContract& contract, std::int64_t contracts);
        void SellCall(const core::OptionContract& contract, std::int64_t contracts);
        void Close(const core::OptionContract& contract);
        void Assign(const core::OptionContract& contract);

    private:
        explicit Portfolio(const core::Money& cash);

        void Open(const core::OptionContract& contract, std::int64_t contracts);
        void Deliver(std::int64_t quantity);

        core::Money cash;
        std::vector<ShareLot> lots;
        std::vector<ShortOption> shorts;
    };

    core::Money Equity(const Portfolio& portfolio, const core::Money& spot);
}
