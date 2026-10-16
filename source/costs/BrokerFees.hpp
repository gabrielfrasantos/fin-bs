#pragma once

#include "source/core/Money.hpp"
#include "source/costs/FeeSchedule.hpp"
#include "source/costs/Trade.hpp"
#include <cstddef>
#include <span>

namespace fin::costs
{
    class BrokerFees
    {
    public:
        explicit BrokerFees(const FeeSchedule& schedule);

        core::Money For(const Trade& trade) const;
        core::Money For(std::span<const Trade> trades) const;
        core::Money ConnectivityFor(const core::Money& portfolioValue, std::size_t exchanges) const;

    private:
        FeeSchedule schedule;
    };
}
