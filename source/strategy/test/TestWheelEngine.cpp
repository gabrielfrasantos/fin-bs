#include "source/strategy/WheelEngine.hpp"
#include <array>
#include <gtest/gtest.h>
#include <optional>

namespace
{
    using fin::strategy::WheelEngine;
    using fin::strategy::WheelEvent;
    using fin::strategy::WheelState;

    constexpr std::array states{ WheelState::Cash, WheelState::ShortPut, WheelState::HoldingShares, WheelState::ShortCall };
    constexpr std::array events{ WheelEvent::SellPut, WheelEvent::SellCall, WheelEvent::ExpiredWorthless, WheelEvent::Assigned, WheelEvent::Closed };

    const std::array<std::array<std::optional<WheelState>, events.size()>, states.size()> expected{ {
        { WheelState::ShortPut, std::nullopt, std::nullopt, std::nullopt, std::nullopt },
        { std::nullopt, std::nullopt, WheelState::Cash, WheelState::HoldingShares, WheelState::Cash },
        { std::nullopt, WheelState::ShortCall, std::nullopt, std::nullopt, std::nullopt },
        { std::nullopt, std::nullopt, WheelState::HoldingShares, WheelState::Cash, WheelState::HoldingShares },
    } };
}

TEST(TestWheelEngine, the_transition_table_is_exactly_the_wheel_and_nothing_else)
{
    for (std::size_t state = 0; state != states.size(); ++state)
        for (std::size_t event = 0; event != events.size(); ++event)
            EXPECT_EQ(WheelEngine::Next(states[state], events[event]), expected[state][event])
                << "state " << state << " event " << event;
}

TEST(TestWheelEngine, an_option_is_only_written_when_nothing_is_open)
{
    EXPECT_FALSE(WheelEngine::Next(WheelState::ShortPut, WheelEvent::SellPut).has_value());
    EXPECT_FALSE(WheelEngine::Next(WheelState::ShortCall, WheelEvent::SellCall).has_value());
}

TEST(TestWheelEngine, a_put_is_never_written_against_shares_and_a_call_is_never_written_against_cash)
{
    EXPECT_FALSE(WheelEngine::Next(WheelState::HoldingShares, WheelEvent::SellPut).has_value());
    EXPECT_FALSE(WheelEngine::Next(WheelState::Cash, WheelEvent::SellCall).has_value());
}

TEST(TestWheelEngine, the_wheel_turns_from_cash_through_shares_and_back_to_cash)
{
    auto state = WheelState::Cash;

    for (const auto event : { WheelEvent::SellPut, WheelEvent::Assigned, WheelEvent::SellCall, WheelEvent::Assigned })
    {
        const auto next = WheelEngine::Next(state, event);
        ASSERT_TRUE(next.has_value());
        state = *next;
    }

    EXPECT_EQ(state, WheelState::Cash);
}

TEST(TestWheelEngine, an_option_that_expires_worthless_leaves_the_wheel_where_it_started)
{
    EXPECT_EQ(WheelEngine::Next(WheelState::ShortPut, WheelEvent::ExpiredWorthless), WheelState::Cash);
    EXPECT_EQ(WheelEngine::Next(WheelState::ShortCall, WheelEvent::ExpiredWorthless), WheelState::HoldingShares);
}

TEST(TestWheelEngine, closing_a_position_early_ends_where_expiring_worthless_would_have)
{
    for (const auto state : { WheelState::ShortPut, WheelState::ShortCall })
        EXPECT_EQ(WheelEngine::Next(state, WheelEvent::Closed), WheelEngine::Next(state, WheelEvent::ExpiredWorthless));
}
