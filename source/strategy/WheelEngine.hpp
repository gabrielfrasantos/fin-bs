#pragma once

#include <cstdint>
#include <optional>

namespace fin::strategy
{
    enum class WheelState : std::uint8_t
    {
        Cash,
        ShortPut,
        HoldingShares,
        ShortCall
    };

    enum class WheelEvent : std::uint8_t
    {
        SellPut,
        SellCall,
        ExpiredWorthless,
        Assigned,
        Closed
    };

    class WheelEngine
    {
    public:
        static std::optional<WheelState> Next(WheelState state, WheelEvent event);
    };
}
