#include "source/sim/GbmPathGenerator.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::sim::GbmParameters;
    using fin::sim::GbmPathGenerator;
    using fin::sim::Rng;

    constexpr std::uint64_t seed = 20250811u;
    constexpr auto initialPrice = 100.00;
    constexpr auto drift = 0.08;
    constexpr auto volatility = 0.20;
    constexpr auto horizon = 1.0;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    GbmPathGenerator Underlying()
    {
        return GbmPathGenerator(GbmParameters{ .initialPrice = Euro(initialPrice),
            .drift = Rate::FromContinuous(drift),
            .volatility = Volatility::FromAnnualized(volatility),
            .convention = DayCountConvention::Act365Fixed });
    }

    std::vector<Date> OverTheHorizon()
    {
        return { Date::FromYearMonthDay(2025, 1, 2), Date::FromYearMonthDay(2026, 1, 2) };
    }

    double ExpectedTerminalPrice()
    {
        return initialPrice * std::exp(drift * horizon);
    }

    double StandardErrorOfTheMean(std::size_t paths)
    {
        const auto deviation = ExpectedTerminalPrice() * std::sqrt(std::expm1(volatility * volatility * horizon));

        return deviation / std::sqrt(static_cast<double>(paths));
    }

    double MeanTerminalPrice(std::size_t paths)
    {
        Rng rng(seed);
        const auto generator = Underlying();
        const auto schedule = OverTheHorizon();
        auto total = 0.0;

        for (std::size_t path = 0; path != paths; ++path)
            total += generator.Along(schedule, rng).Closes().back().ToDouble();

        return total / static_cast<double>(paths);
    }

    double ErrorOfTheMean(std::size_t paths)
    {
        return std::abs(MeanTerminalPrice(paths) - ExpectedTerminalPrice());
    }
}

TEST(TestConvergence, the_simulated_mean_stays_within_three_standard_errors_of_the_closed_form)
{
    for (const std::size_t paths : { 1000u, 10000u, 100000u })
        EXPECT_LT(ErrorOfTheMean(paths), 3.0 * StandardErrorOfTheMean(paths)) << "with " << paths << " paths";
}

TEST(TestConvergence, a_hundred_times_as_many_paths_leaves_a_smaller_error)
{
    EXPECT_LT(ErrorOfTheMean(100000), ErrorOfTheMean(1000));
}
