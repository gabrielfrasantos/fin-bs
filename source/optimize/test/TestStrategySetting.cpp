#include "source/optimize/StrategySetting.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Delta;
    using fin::optimize::Point;
    using fin::optimize::PointOf;
    using fin::optimize::SettingAt;
    using fin::optimize::SettingRange;
    using fin::optimize::StrategySetting;

    StrategySetting Setting(double targetDelta, std::int32_t buffer, double rollLimit)
    {
        return { .targetDelta = Delta::FromValue(targetDelta), .cashBufferBasisPoints = buffer, .rollDeltaLimit = Delta::FromValue(rollLimit) };
    }

    SettingRange Range()
    {
        return { .lowest = Setting(0.10, 200, 0.40), .highest = Setting(0.50, 4200, 0.80) };
    }
}

TEST(TestStrategySetting, a_setting_inside_its_bounds_is_well_formed)
{
    EXPECT_TRUE(IsWellFormed(Setting(0.30, 500, 0.60)));
    EXPECT_TRUE(IsWellFormed(Setting(0.01, 0, 0.99)));
    EXPECT_TRUE(IsWellFormed(Setting(0.99, 10000, 1.00)));
}

TEST(TestStrategySetting, a_setting_outside_its_bounds_is_not_well_formed)
{
    EXPECT_FALSE(IsWellFormed(Setting(1.00, 500, 0.60)));
    EXPECT_FALSE(IsWellFormed(Setting(-0.30, 500, 0.60)));
    EXPECT_FALSE(IsWellFormed(Setting(0.30, -1, 0.60)));
    EXPECT_FALSE(IsWellFormed(Setting(0.30, 10001, 0.60)));
    EXPECT_FALSE(IsWellFormed(Setting(0.30, 500, -0.60)));
}

TEST(TestStrategySetting, a_range_must_grow_in_every_coordinate)
{
    EXPECT_TRUE(IsWellFormed(Range()));
    EXPECT_FALSE(IsWellFormed(SettingRange{ .lowest = Setting(0.50, 200, 0.40), .highest = Setting(0.50, 4200, 0.80) }));
    EXPECT_FALSE(IsWellFormed(SettingRange{ .lowest = Setting(0.10, 4200, 0.40), .highest = Setting(0.50, 4200, 0.80) }));
    EXPECT_FALSE(IsWellFormed(SettingRange{ .lowest = Setting(0.10, 200, 0.90), .highest = Setting(0.50, 4200, 0.80) }));
}

TEST(TestStrategySetting, the_corners_of_the_unit_box_are_the_ends_of_the_range)
{
    const auto lowest = SettingAt(Range(), Point{ 0.0, 0.0, 0.0 });
    const auto highest = SettingAt(Range(), Point{ 1.0, 1.0, 1.0 });

    EXPECT_DOUBLE_EQ(lowest.targetDelta.Value(), 0.10);
    EXPECT_EQ(lowest.cashBufferBasisPoints, 200);
    EXPECT_DOUBLE_EQ(lowest.rollDeltaLimit.Value(), 0.40);
    EXPECT_DOUBLE_EQ(highest.targetDelta.Value(), 0.50);
    EXPECT_EQ(highest.cashBufferBasisPoints, 4200);
    EXPECT_DOUBLE_EQ(highest.rollDeltaLimit.Value(), 0.80);
}

TEST(TestStrategySetting, the_centre_of_the_unit_box_is_the_middle_of_the_range)
{
    const auto middle = SettingAt(Range(), Point{ 0.5, 0.5, 0.5 });

    EXPECT_DOUBLE_EQ(middle.targetDelta.Value(), 0.30);
    EXPECT_EQ(middle.cashBufferBasisPoints, 2200);
    EXPECT_DOUBLE_EQ(middle.rollDeltaLimit.Value(), 0.60);
}

TEST(TestStrategySetting, a_coordinate_beyond_a_face_of_the_box_reads_as_the_end_of_the_range)
{
    EXPECT_EQ(SettingAt(Range(), Point{ -3.0, 2.0, 0.5 }).cashBufferBasisPoints, 4200);
    EXPECT_DOUBLE_EQ(SettingAt(Range(), Point{ -3.0, 2.0, 0.5 }).targetDelta.Value(), 0.10);
}

TEST(TestStrategySetting, a_setting_survives_the_journey_to_the_unit_box_and_back)
{
    const auto setting = Setting(0.35, 1700, 0.75);
    const auto returned = SettingAt(Range(), PointOf(Range(), setting));

    EXPECT_DOUBLE_EQ(returned.targetDelta.Value(), setting.targetDelta.Value());
    EXPECT_EQ(returned.cashBufferBasisPoints, setting.cashBufferBasisPoints);
    EXPECT_DOUBLE_EQ(returned.rollDeltaLimit.Value(), setting.rollDeltaLimit.Value());
}

TEST(TestStrategySetting, a_setting_outside_the_range_is_placed_on_the_face_it_left_through)
{
    EXPECT_EQ(PointOf(Range(), Setting(0.05, 100, 0.90)), (Point{ 0.0, 0.0, 1.0 }));
}
