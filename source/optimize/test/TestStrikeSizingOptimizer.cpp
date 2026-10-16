#include "source/optimize/StrikeSizingOptimizer.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    using fin::optimize::Descent;
    using fin::optimize::DescentTerms;
    using fin::optimize::Objective;
    using fin::optimize::Point;
    using fin::optimize::StrikeSizingOptimizer;

    constexpr Point start{ 0.5, 0.5, 0.5 };
    constexpr Point floorOfTheBowl{ 0.2, 0.8, 0.35 };
    constexpr double curvature = 1.0;

    class Bowl
        : public Objective
    {
    public:
        Bowl(const Point& optimum, double curvature)
            : optimum(optimum)
            , curvature(curvature)
        {}

        double Cost(const Point& at) const override
        {
            auto total = 0.0;

            for (std::size_t coordinate = 0; coordinate != at.size(); ++coordinate)
                total += (at[coordinate] - optimum[coordinate]) * (at[coordinate] - optimum[coordinate]);

            return 0.5 * curvature * total;
        }

        Point Gradient(const Point& at) const override
        {
            Point gradient{};

            for (std::size_t coordinate = 0; coordinate != at.size(); ++coordinate)
                gradient[coordinate] = curvature * (at[coordinate] - optimum[coordinate]);

            return gradient;
        }

    private:
        Point optimum;
        double curvature;
    };

    DescentTerms Terms(double sparsityWeight, double shrinkageWeight)
    {
        return { .learningRate = 0.2, .iterations = 512, .sparsityWeight = sparsityWeight, .shrinkageWeight = shrinkageWeight };
    }

    StrikeSizingOptimizer Built(const DescentTerms& terms)
    {
        return StrikeSizingOptimizer(terms);
    }

    void ExpectPointsAgree(const Point& found, const Point& expected)
    {
        for (std::size_t coordinate = 0; coordinate != found.size(); ++coordinate)
            EXPECT_NEAR(found[coordinate], expected[coordinate], 1e-9) << "coordinate " << coordinate;
    }
}

TEST(TestStrikeSizingOptimizer, terms_that_describe_a_descent_are_well_formed)
{
    EXPECT_TRUE(IsWellFormed(Terms(0.0, 0.0)));
    EXPECT_FALSE(IsWellFormed(DescentTerms{ .learningRate = 0.0, .iterations = 512, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 }));
    EXPECT_FALSE(IsWellFormed(DescentTerms{ .learningRate = 0.2, .iterations = 0, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 }));
    EXPECT_FALSE(IsWellFormed(DescentTerms{ .learningRate = 0.2, .iterations = 512, .sparsityWeight = -1.0, .shrinkageWeight = 0.0 }));
    EXPECT_FALSE(IsWellFormed(DescentTerms{ .learningRate = 0.2, .iterations = 512, .sparsityWeight = 0.0, .shrinkageWeight = -1.0 }));
}

TEST(TestStrikeSizingOptimizer, terms_that_do_not_describe_a_descent_are_refused)
{
    EXPECT_DEATH(Built(DescentTerms{ .learningRate = -0.2, .iterations = 512, .sparsityWeight = 0.0, .shrinkageWeight = 0.0 }), "");
}

TEST(TestStrikeSizingOptimizer, the_floor_of_a_convex_bowl_is_found)
{
    const Bowl bowl(floorOfTheBowl, curvature);

    const auto descent = Built(Terms(0.0, 0.0)).From(start, bowl);

    ExpectPointsAgree(descent.at, floorOfTheBowl);
    EXPECT_NEAR(descent.cost, 0.0, 1e-15);
    EXPECT_EQ(descent.iterations, 512u);
}

TEST(TestStrikeSizingOptimizer, the_reported_cost_is_the_cost_of_the_reported_point)
{
    const Bowl bowl(floorOfTheBowl, curvature);

    const auto descent = Built(Terms(0.1, 0.5)).From(start, bowl);

    EXPECT_DOUBLE_EQ(descent.cost, bowl.Cost(descent.at));
}

TEST(TestStrikeSizingOptimizer, a_bowl_whose_floor_lies_outside_the_box_is_answered_on_the_face_nearest_to_it)
{
    const Bowl bowl(Point{ 3.0, -2.0, 0.35 }, curvature);

    const auto descent = Built(Terms(0.0, 0.0)).From(start, bowl);

    ExpectPointsAgree(descent.at, Point{ 1.0, 0.0, 0.35 });
}

TEST(TestStrikeSizingOptimizer, shrinkage_settles_between_the_floor_of_the_bowl_and_the_setting_it_started_from)
{
    constexpr auto weight = 3.0;
    const Bowl bowl(floorOfTheBowl, curvature);

    const auto descent = Built(Terms(0.0, weight)).From(start, bowl);

    ExpectPointsAgree(descent.at, Point{ (curvature * floorOfTheBowl[0] + weight * start[0]) / (curvature + weight),
                                      (curvature * floorOfTheBowl[1] + weight * start[1]) / (curvature + weight),
                                      (curvature * floorOfTheBowl[2] + weight * start[2]) / (curvature + weight) });
}

TEST(TestStrikeSizingOptimizer, sparsity_moves_every_coordinate_the_same_distance_short_of_the_floor_of_the_bowl)
{
    constexpr auto weight = 0.1;
    const Bowl bowl(floorOfTheBowl, curvature);

    const auto descent = Built(Terms(weight, 0.0)).From(start, bowl);

    ExpectPointsAgree(descent.at, Point{ floorOfTheBowl[0] + weight / curvature, floorOfTheBowl[1] - weight / curvature, floorOfTheBowl[2] + weight / curvature });
}

TEST(TestStrikeSizingOptimizer, the_heavier_the_sparsity_the_less_the_setting_moves)
{
    const Bowl bowl(floorOfTheBowl, curvature);

    const auto light = Built(Terms(0.05, 0.0)).From(start, bowl);
    const auto heavy = Built(Terms(0.12, 0.0)).From(start, bowl);

    for (std::size_t coordinate = 0; coordinate != start.size(); ++coordinate)
        EXPECT_LT(std::abs(heavy.at[coordinate] - start[coordinate]), std::abs(light.at[coordinate] - start[coordinate])) << "coordinate " << coordinate;
}
