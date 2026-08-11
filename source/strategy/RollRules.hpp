#pragma once

#include "source/core/Delta.hpp"
#include <cstdint>
#include <optional>

namespace fin::strategy
{
    enum class RollReason : std::uint8_t
    {
        DeltaBreached,
        ProfitCaptured,
        NearExpiry
    };

    struct RollTriggers
    {
        std::int32_t daysToExpiry;
        std::int32_t profitCaptureBasisPoints;
        core::Delta deltaLimit;
    };

    struct PositionObservation
    {
        std::int32_t daysToExpiry;
        double premiumReceivedPerShare;
        double valuePerShare;
        core::Delta delta;
    };

    class RollRules
    {
    public:
        explicit RollRules(const RollTriggers& triggers);

        std::optional<RollReason> Triggered(const PositionObservation& observation) const;
        bool Allows(const core::Delta& replacement) const;

    private:
        RollTriggers triggers;
    };

    bool IsWellFormed(const RollTriggers& triggers);
    std::int32_t CapturedBasisPoints(const PositionObservation& observation);
}
