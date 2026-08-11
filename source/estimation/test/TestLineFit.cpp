#include "source/estimation/LineFit.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
#include "source/estimation/TotalLeastSquaresFit.hpp"
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::estimation::LineFit;
    using fin::estimation::OrdinaryLeastSquaresFit;
    using fin::estimation::TotalLeastSquaresFit;

    std::vector<double> Ramp(std::size_t count, double first, double step)
    {
        std::vector<double> result;

        for (std::size_t i = 0; i != count; ++i)
            result.push_back(first + step * static_cast<double>(i));

        return result;
    }

    std::vector<double> Shifted(std::vector<double> values, double offset)
    {
        for (auto& value : values)
            value += offset;

        return values;
    }

    std::vector<double> Amplified(std::vector<double> values, double factor)
    {
        for (auto& value : values)
            value *= factor;

        return values;
    }

    class TestLineFit
        : public ::testing::Test
    {
    protected:
        OrdinaryLeastSquaresFit ordinary;
        TotalLeastSquaresFit orthogonal;

        std::vector<const LineFit*> Both() const
        {
            return { &ordinary, &orthogonal };
        }
    };
}

TEST_F(TestLineFit, every_fit_recovers_the_slope_of_a_straight_line)
{
    const auto abscissa = Ramp(400, -3.0, 0.25);
    const auto ordinate = Amplified(Shifted(abscissa, 0.0), 1.75);

    for (const auto* fit : Both())
        EXPECT_NEAR(fit->Slope(abscissa, Shifted(ordinate, 12.0)).value(), 1.75, 1e-9);
}

TEST_F(TestLineFit, every_fit_reports_the_capacity_it_will_accept)
{
    for (const auto* fit : Both())
        EXPECT_GE(fit->MaximumObservations(), 8192u);
}

TEST_F(TestLineFit, a_fit_uses_only_the_observations_it_was_given)
{
    const auto shortAbscissa = Ramp(6, 0.0, 1.0);
    const auto longAbscissa = Ramp(600, 0.0, 1.0);

    for (const auto* fit : Both())
        EXPECT_NEAR(fit->Slope(shortAbscissa, Amplified(shortAbscissa, -0.4)).value(),
            fit->Slope(longAbscissa, Amplified(longAbscissa, -0.4)).value(), 1e-9);
}

TEST_F(TestLineFit, shifting_either_axis_leaves_the_slope_unchanged)
{
    const auto abscissa = Ramp(100, 1.0, 0.5);
    const auto ordinate = Amplified(abscissa, 0.3);

    for (const auto* fit : Both())
    {
        const auto slope = fit->Slope(abscissa, ordinate).value();

        EXPECT_NEAR(fit->Slope(Shifted(abscissa, 1000.0), ordinate).value(), slope, 1e-9);
        EXPECT_NEAR(fit->Slope(abscissa, Shifted(ordinate, -1000.0)).value(), slope, 1e-9);
    }
}

TEST_F(TestLineFit, stretching_an_axis_scales_the_slope_by_that_factor)
{
    const auto abscissa = Ramp(100, 0.0, 0.5);
    const auto ordinate = Amplified(abscissa, 0.3);

    for (const auto* fit : Both())
    {
        const auto slope = fit->Slope(abscissa, ordinate).value();

        EXPECT_NEAR(fit->Slope(abscissa, Amplified(ordinate, 10.0)).value(), 10.0 * slope, 1e-9);
        EXPECT_NEAR(fit->Slope(Amplified(abscissa, 10.0), ordinate).value(), 0.1 * slope, 1e-9);
    }
}

TEST_F(TestLineFit, an_ordinate_that_never_changes_has_a_slope_of_exactly_zero)
{
    const auto abscissa = Ramp(50, 0.0, 1.0);
    const std::vector<double> flat(50, 7.0);

    for (const auto* fit : Both())
        EXPECT_EQ(fit->Slope(abscissa, flat).value(), 0.0);
}

TEST_F(TestLineFit, an_abscissa_that_never_changes_has_no_slope_at_all)
{
    const std::vector<double> flat(50, 7.0);
    const auto ordinate = Ramp(50, 0.0, 1.0);

    for (const auto* fit : Both())
        EXPECT_FALSE(fit->Slope(flat, ordinate).has_value());
}

TEST_F(TestLineFit, the_orthogonal_fit_is_the_steeper_one_when_the_ordinate_is_noisy)
{
    const auto abscissa = Ramp(200, 0.0, 1.0);
    auto ordinate = Amplified(abscissa, 0.5);

    for (std::size_t i = 0; i != ordinate.size(); ++i)
        ordinate[i] += i % 2 == 0 ? 20.0 : -20.0;

    EXPECT_GT(orthogonal.Slope(abscissa, ordinate).value(), ordinary.Slope(abscissa, ordinate).value());
}

TEST_F(TestLineFit, mismatched_or_insufficient_observations_stop_the_run)
{
    const auto abscissa = Ramp(10, 0.0, 1.0);
    const auto ordinate = Ramp(9, 0.0, 1.0);
    const auto single = Ramp(1, 0.0, 1.0);

    for (const auto* fit : Both())
    {
        EXPECT_DEATH(fit->Slope(abscissa, ordinate), "");
        EXPECT_DEATH(fit->Slope(single, single), "");
        EXPECT_DEATH(fit->Slope({}, {}), "");
    }
}

TEST_F(TestLineFit, more_observations_than_the_fit_admits_stop_the_run)
{
    for (const auto* fit : Both())
    {
        const auto tooMany = Ramp(fit->MaximumObservations() + 1, 0.0, 1.0);

        EXPECT_DEATH(fit->Slope(tooMany, tooMany), "");
    }
}
