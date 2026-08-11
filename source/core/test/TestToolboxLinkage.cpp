#include "numerical/math/Math.hpp"
#include <gtest/gtest.h>
#include <numbers>

namespace
{
    // Guards the numerical-toolbox submodule wiring: the Black-Scholes normal CDF
    // (see PLAN.md T2.1) is built on math::Erfc, so linkage is checked through it.
    float StandardNormalCdf(float x)
    {
        return 0.5f * math::Erfc(-x / std::numbers::sqrt2_v<float>);
    }

    class TestToolboxLinkage
        : public ::testing::Test
    {
    protected:
        static constexpr float epsilon = 1e-6f;
    };
}

TEST_F(TestToolboxLinkage, standard_normal_cdf_at_zero_is_one_half)
{
    EXPECT_NEAR(StandardNormalCdf(0.0f), 0.5f, epsilon);
}

TEST_F(TestToolboxLinkage, standard_normal_cdf_is_symmetric)
{
    EXPECT_NEAR(StandardNormalCdf(-1.28f), 1.0f - StandardNormalCdf(1.28f), epsilon);
}
