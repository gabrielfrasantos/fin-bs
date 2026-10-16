#include "source/core/Delta.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Delta;

    class TestDelta
        : public ::testing::Test
    {};
}

TEST_F(TestDelta, keeps_the_signed_value_and_reports_its_magnitude)
{
    EXPECT_DOUBLE_EQ(Delta::FromValue(0.30).Value(), 0.30);
    EXPECT_DOUBLE_EQ(Delta::FromValue(-0.30).Value(), -0.30);
    EXPECT_DOUBLE_EQ(Delta::FromValue(-0.30).Magnitude(), 0.30);
    EXPECT_DOUBLE_EQ(Delta::FromValue(0.30).Magnitude(), 0.30);
}

TEST_F(TestDelta, negation_round_trips)
{
    const auto delta = Delta::FromValue(0.42);

    EXPECT_TRUE(-(-delta) == delta);
    EXPECT_DOUBLE_EQ((-delta).Value(), -0.42);
}

TEST_F(TestDelta, accepts_the_full_closed_range)
{
    EXPECT_DOUBLE_EQ(Delta::FromValue(1.0).Value(), 1.0);
    EXPECT_DOUBLE_EQ(Delta::FromValue(-1.0).Value(), -1.0);
    EXPECT_DOUBLE_EQ(Delta::FromValue(0.0).Value(), 0.0);
}

TEST_F(TestDelta, orders_by_the_signed_value)
{
    EXPECT_TRUE(Delta::FromValue(-0.30) < Delta::FromValue(0.30));
    EXPECT_TRUE(Delta::FromValue(0.30) == Delta::FromValue(0.30));
}

TEST_F(TestDelta, is_usable_in_a_constant_expression)
{
    static_assert(Delta::FromValue(-0.30).Magnitude() == 0.30);
}

TEST_F(TestDelta, rejects_values_outside_the_unit_interval)
{
    EXPECT_DEATH(static_cast<void>(Delta::FromValue(1.01)), "");
    EXPECT_DEATH(static_cast<void>(Delta::FromValue(-1.01)), "");
}
