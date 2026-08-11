#include "source/strategy/WheelEngine.hpp"

namespace fin::strategy
{
    std::optional<WheelState> WheelEngine::Next(WheelState state, WheelEvent event)
    {
        switch (state)
        {
            case WheelState::Cash:
                if (event == WheelEvent::SellPut)
                    return WheelState::ShortPut;
                return std::nullopt;

            case WheelState::ShortPut:
                if (event == WheelEvent::Assigned)
                    return WheelState::HoldingShares;
                if (event == WheelEvent::ExpiredWorthless || event == WheelEvent::Closed)
                    return WheelState::Cash;
                return std::nullopt;

            case WheelState::HoldingShares:
                if (event == WheelEvent::SellCall)
                    return WheelState::ShortCall;
                return std::nullopt;

            default:
                if (event == WheelEvent::Assigned)
                    return WheelState::Cash;
                if (event == WheelEvent::ExpiredWorthless || event == WheelEvent::Closed)
                    return WheelState::HoldingShares;
                return std::nullopt;
        }
    }
}
