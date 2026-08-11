#include "source/sim/Rng.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::sim::Rng;

    constexpr std::size_t manyDraws = 1000000;

    std::vector<double> Normals(std::uint64_t seed, std::size_t draws)
    {
        Rng rng(seed);
        std::vector<double> values;
        values.reserve(draws);

        for (std::size_t draw = 0; draw != draws; ++draw)
            values.push_back(rng.NextStandardNormal());

        return values;
    }
}

TEST(TestRng, the_same_seed_produces_the_same_stream)
{
    EXPECT_EQ(Normals(20250811u, 1000), Normals(20250811u, 1000));
}

TEST(TestRng, a_different_seed_produces_a_different_stream)
{
    EXPECT_NE(Normals(1u, 1000), Normals(2u, 1000));
}

TEST(TestRng, a_uniform_lies_strictly_between_zero_and_one)
{
    Rng rng(7u);

    for (std::size_t draw = 0; draw != 100000; ++draw)
    {
        const auto uniform = rng.NextUniform();

        ASSERT_GT(uniform, 0.0);
        ASSERT_LT(uniform, 1.0);
    }
}

TEST(TestRng, uniforms_average_a_half)
{
    Rng rng(11u);
    auto total = 0.0;

    for (std::size_t draw = 0; draw != manyDraws; ++draw)
        total += rng.NextUniform();

    EXPECT_NEAR(total / static_cast<double>(manyDraws), 0.5, 0.002);
}

TEST(TestRng, normals_have_a_mean_of_zero_and_a_variance_of_one)
{
    const auto values = Normals(3u, manyDraws);

    auto total = 0.0;
    for (const auto value : values)
        total += value;

    const auto mean = total / static_cast<double>(manyDraws);

    auto squares = 0.0;
    for (const auto value : values)
        squares += (value - mean) * (value - mean);

    EXPECT_NEAR(mean, 0.0, 0.005);
    EXPECT_NEAR(squares / static_cast<double>(manyDraws - 1), 1.0, 0.01);
}

TEST(TestRng, normals_are_drawn_in_pairs_that_are_both_returned)
{
    Rng first(5u);
    Rng second(5u);

    const auto lead = first.NextStandardNormal();
    const auto follow = first.NextStandardNormal();

    EXPECT_EQ(second.NextStandardNormal(), lead);
    EXPECT_EQ(second.NextStandardNormal(), follow);
    EXPECT_NE(lead, follow);
}
