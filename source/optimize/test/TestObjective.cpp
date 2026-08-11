#include "source/optimize/Objective.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::optimize::ClampedToTheUnitBox;
    using fin::optimize::IsWithinTheUnitBox;
    using fin::optimize::Point;
}

TEST(TestObjective, the_corners_of_the_unit_box_are_inside_it)
{
    EXPECT_TRUE(IsWithinTheUnitBox(Point{ 0.0, 0.0, 0.0 }));
    EXPECT_TRUE(IsWithinTheUnitBox(Point{ 1.0, 1.0, 1.0 }));
    EXPECT_TRUE(IsWithinTheUnitBox(Point{ 0.25, 0.5, 0.75 }));
}

TEST(TestObjective, a_point_beyond_any_face_is_outside_the_unit_box)
{
    EXPECT_FALSE(IsWithinTheUnitBox(Point{ -0.001, 0.5, 0.5 }));
    EXPECT_FALSE(IsWithinTheUnitBox(Point{ 0.5, 1.001, 0.5 }));
    EXPECT_FALSE(IsWithinTheUnitBox(Point{ 0.5, 0.5, -1.0 }));
}

TEST(TestObjective, clamping_moves_a_point_to_the_nearest_face_and_leaves_the_others_alone)
{
    const auto clamped = ClampedToTheUnitBox(Point{ -2.0, 0.375, 8.0 });

    EXPECT_EQ(clamped, (Point{ 0.0, 0.375, 1.0 }));
    EXPECT_TRUE(IsWithinTheUnitBox(clamped));
}

TEST(TestObjective, clamping_a_point_that_is_already_inside_changes_nothing)
{
    const Point inside{ 0.1, 0.2, 0.3 };

    EXPECT_EQ(ClampedToTheUnitBox(inside), inside);
}
