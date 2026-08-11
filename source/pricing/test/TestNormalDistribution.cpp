#include "source/pricing/NormalDistribution.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    using fin::pricing::StandardNormalCdf;
    using fin::pricing::StandardNormalPdf;

    class TestNormalDistribution
        : public ::testing::Test
    {
    protected:
        static constexpr double publishedTolerance = 1e-9;
    };
}

TEST_F(TestNormalDistribution, cumulative_probability_at_the_mean_is_one_half)
{
    EXPECT_EQ(StandardNormalCdf(0.0), 0.5);
}

TEST_F(TestNormalDistribution, cumulative_probability_is_symmetric_about_the_mean)
{
    for (auto x : { 0.1, 0.5, 1.0, 1.96, 3.0, 7.5 })
        EXPECT_NEAR(StandardNormalCdf(-x), 1.0 - StandardNormalCdf(x), 1e-15);
}

TEST_F(TestNormalDistribution, cumulative_probability_matches_published_table_values)
{
    EXPECT_NEAR(StandardNormalCdf(-1.0), 0.1586552539, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(1.0), 0.8413447461, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(1.96), 0.9750021049, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(2.5), 0.9937903347, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(3.0), 0.9986501020, publishedTolerance);
}

TEST_F(TestNormalDistribution, cumulative_probability_matches_published_quantiles)
{
    EXPECT_NEAR(StandardNormalCdf(-2.3263478740), 0.01, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(1.2815515655), 0.90, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(1.6448536270), 0.95, publishedTolerance);
    EXPECT_NEAR(StandardNormalCdf(1.9599639845), 0.975, publishedTolerance);
}

TEST_F(TestNormalDistribution, cumulative_probability_increases_and_saturates_at_the_tails)
{
    double previous = 0.0;

    for (double x = -8.0; x <= 8.0; x += 0.25)
    {
        const double probability = StandardNormalCdf(x);

        EXPECT_GE(probability, previous);
        EXPECT_GE(probability, 0.0);
        EXPECT_LE(probability, 1.0);

        previous = probability;
    }

    EXPECT_EQ(StandardNormalCdf(-40.0), 0.0);
    EXPECT_EQ(StandardNormalCdf(40.0), 1.0);
}

TEST_F(TestNormalDistribution, density_matches_published_table_values)
{
    EXPECT_NEAR(StandardNormalPdf(0.0), 0.3989422804, publishedTolerance);
    EXPECT_NEAR(StandardNormalPdf(1.0), 0.2419707245, publishedTolerance);
    EXPECT_NEAR(StandardNormalPdf(2.0), 0.0539909665, publishedTolerance);
    EXPECT_NEAR(StandardNormalPdf(3.0), 0.0044318484, publishedTolerance);
}

TEST_F(TestNormalDistribution, density_is_symmetric_about_the_mean)
{
    for (auto x : { 0.1, 0.5, 1.0, 1.96, 3.0 })
        EXPECT_EQ(StandardNormalPdf(-x), StandardNormalPdf(x));
}

TEST_F(TestNormalDistribution, density_is_the_slope_of_the_cumulative_probability)
{
    constexpr double step = 1e-5;

    for (auto x : { -2.0, -0.5, 0.0, 0.7, 1.5 })
        EXPECT_NEAR((StandardNormalCdf(x + step) - StandardNormalCdf(x - step)) / (2.0 * step), StandardNormalPdf(x), 1e-9);
}
