#include "source/costs/BrokerFees.hpp"
#include "infra/util/ReallyAssert.hpp"
#include <algorithm>

namespace fin::costs
{
    namespace
    {
        constexpr std::int64_t basisPointsPerUnit = 10000;
        constexpr std::int64_t daysPerYear = 365;
    }

    BrokerFees::BrokerFees(const FeeSchedule& schedule)
        : schedule(schedule)
    {
        really_assert(IsWellFormed(schedule));
    }

    core::Money BrokerFees::For(const Trade& trade) const
    {
        if (trade.Traded() == Instrument::Option)
            return schedule.optionFeePerContract * trade.Quantity();

        return schedule.shareCommissionPerTrade + schedule.handlingFeePerTrade;
    }

    core::Money BrokerFees::For(std::span<const Trade> trades) const
    {
        auto total = core::Money::Zero(CurrencyOf(schedule));

        for (const auto& trade : trades)
            total += For(trade);

        return total;
    }

    core::Money BrokerFees::ConnectivityFor(const core::Money& portfolioValue, std::size_t exchanges) const
    {
        really_assert(portfolioValue.GetCurrency() == CurrencyOf(schedule));
        really_assert(portfolioValue.Cents() >= 0);

        const auto proportional = (portfolioValue * static_cast<std::int64_t>(schedule.connectivityBasisPoints)) / basisPointsPerUnit;

        return std::min(proportional, schedule.connectivityFeeCapPerExchangePerYear) * static_cast<std::int64_t>(exchanges);
    }

    core::Money BrokerFees::DebitInterest(const core::Money& balance, std::int32_t days) const
    {
        really_assert(balance.GetCurrency() == CurrencyOf(schedule));
        really_assert(days >= 0);

        if (balance.Cents() >= 0)
            return core::Money::Zero(CurrencyOf(schedule));

        return ((-balance) * static_cast<std::int64_t>(schedule.debitInterestBasisPoints) * static_cast<std::int64_t>(days)) / (basisPointsPerUnit * daysPerYear);
    }
}
