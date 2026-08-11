#pragma once

#include "source/core/Date.hpp"
#include "source/core/Delta.hpp"
#include "source/core/Money.hpp"
#include "source/core/OptionContract.hpp"
#include "source/core/Rate.hpp"
#include "source/core/Volatility.hpp"
#include "source/estimation/PriceSeries.hpp"
#include "source/strategy/Projection.hpp"
#include "source/strategy/WheelEngine.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace fin::sim
{
    struct AnnualReference
    {
        std::uint32_t month;
        std::uint32_t day;
    };

    struct WheelTerms
    {
        core::Date start;
        std::int32_t cycles;
        std::int32_t daysPerCycle;
        std::int32_t observationDaysBeforeExpiry;
        core::Delta targetDelta;
        core::Money strikeIncrement;
        core::Rate riskFreeRate;
        core::Rate dividendYield;
        core::Volatility volatility;
        std::optional<AnnualReference> assessment;
    };

    class WheelScript
    {
    public:
        explicit WheelScript(const WheelTerms& terms);

        std::int32_t Cycles() const;
        double Horizon() const;
        std::vector<core::Date> Schedule() const;
        strategy::ProjectionPeriod PeriodFor(std::int32_t cycle, strategy::WheelState state, const estimation::PriceSeries& path) const;

    private:
        core::Date Opened(std::int32_t cycle) const;
        core::Date Expiry(std::int32_t cycle) const;
        std::optional<core::Date> Observation(std::int32_t cycle) const;
        std::optional<core::Date> Assessment(std::int32_t cycle) const;
        core::Money StrikeFor(const core::Money& spot, core::OptionType type, double timeToExpiry) const;

        WheelTerms terms;
    };

    bool IsWellFormed(const WheelTerms& terms);
}
