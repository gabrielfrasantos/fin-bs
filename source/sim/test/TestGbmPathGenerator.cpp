#include "source/estimation/MarketParameters.hpp"
#include "source/estimation/OrdinaryLeastSquaresFit.hpp"
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
    using fin::estimation::MarketParameters;
    using fin::estimation::OrdinaryLeastSquaresFit;
    using fin::sim::GbmParameters;
    using fin::sim::GbmPathGenerator;
    using fin::sim::Rng;

    constexpr auto initialPrice = 100.00;
    constexpr auto drift = 0.08;
    constexpr auto volatility = 0.20;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    GbmParameters Parameters(double annualVolatility, DayCountConvention convention)
    {
        return { .initialPrice = Euro(initialPrice),
            .drift = Rate::FromContinuous(drift),
            .volatility = Volatility::FromAnnualized(annualVolatility),
            .convention = convention };
    }

    std::vector<Date> OverOneYear()
    {
        return { On(2025, 1, 2), On(2026, 1, 2) };
    }

    std::vector<Date> BusinessDays(const Date& start, std::size_t observations)
    {
        std::vector<Date> dates;
        auto date = start;

        while (dates.size() != observations)
        {
            if (!date.IsWeekend())
                dates.push_back(date);

            date = date.AddDays(1);
        }

        return dates;
    }

    double MeanTerminalPrice(const GbmPathGenerator& generator, std::size_t paths, Rng& rng)
    {
        const auto schedule = OverOneYear();
        auto total = 0.0;

        for (std::size_t path = 0; path != paths; ++path)
            total += generator.Along(schedule, rng).Closes().back().ToDouble();

        return total / static_cast<double>(paths);
    }
}

TEST(TestGbmPathGenerator, the_path_opens_at_the_initial_price_on_the_first_scheduled_date)
{
    Rng rng(1u);
    const auto path = GbmPathGenerator(Parameters(volatility, DayCountConvention::Act365Fixed)).Along(OverOneYear(), rng);

    EXPECT_EQ(path.Dates().front(), On(2025, 1, 2));
    EXPECT_EQ(path.Closes().front(), Euro(initialPrice));
}

TEST(TestGbmPathGenerator, the_path_is_priced_on_exactly_the_dates_it_was_given)
{
    Rng rng(1u);
    const auto schedule = BusinessDays(On(2025, 1, 2), 60);
    const auto path = GbmPathGenerator(Parameters(volatility, DayCountConvention::Act252)).Along(schedule, rng);

    ASSERT_EQ(path.Size(), schedule.size());

    for (std::size_t step = 0; step != schedule.size(); ++step)
        EXPECT_EQ(path.Dates()[step], schedule[step]);
}

TEST(TestGbmPathGenerator, the_log_drift_is_the_arithmetic_drift_less_half_the_variance)
{
    const auto generator = GbmPathGenerator(Parameters(volatility, DayCountConvention::Act365Fixed));

    EXPECT_DOUBLE_EQ(generator.LogDrift().Continuous(), drift - 0.5 * volatility * volatility);
}

TEST(TestGbmPathGenerator, without_volatility_the_path_is_the_integrated_drift_alone)
{
    Rng rng(1u);
    const auto schedule = BusinessDays(On(2025, 1, 2), 250);
    const auto generator = GbmPathGenerator(Parameters(0.0, DayCountConvention::Act252));
    const auto path = generator.Along(schedule, rng);

    for (std::size_t step = 0; step != path.Size(); ++step)
    {
        const auto elapsed = fin::core::YearFraction(schedule.front(), schedule[step], DayCountConvention::Act252);

        EXPECT_EQ(path.Closes()[step], Euro(initialPrice * generator.LogDrift().GrowthFactor(elapsed)));
    }
}

TEST(TestGbmPathGenerator, the_mean_terminal_price_is_the_initial_price_grown_at_the_drift)
{
    Rng rng(2u);
    const auto mean = MeanTerminalPrice(GbmPathGenerator(Parameters(volatility, DayCountConvention::Act365Fixed)), 20000, rng);

    EXPECT_NEAR(mean, initialPrice * std::exp(drift), 0.5);
}

TEST(TestGbmPathGenerator, the_variance_of_the_terminal_log_price_is_the_variance_over_the_horizon)
{
    Rng rng(3u);
    const auto generator = GbmPathGenerator(Parameters(volatility, DayCountConvention::Act365Fixed));
    const auto schedule = OverOneYear();

    std::vector<double> logPrices;
    for (std::size_t path = 0; path != 20000; ++path)
        logPrices.push_back(std::log(generator.Along(schedule, rng).Closes().back().ToDouble()));

    auto total = 0.0;
    for (const auto logPrice : logPrices)
        total += logPrice;

    const auto mean = total / static_cast<double>(logPrices.size());

    auto squares = 0.0;
    for (const auto logPrice : logPrices)
        squares += (logPrice - mean) * (logPrice - mean);

    EXPECT_NEAR(mean, std::log(initialPrice) + generator.LogDrift().Continuous(), 0.005);
    EXPECT_NEAR(squares / static_cast<double>(logPrices.size() - 1), volatility * volatility, 0.002);
}

TEST(TestGbmPathGenerator, a_simulated_path_returns_the_volatility_it_was_generated_with)
{
    Rng rng(4u);
    const auto path = GbmPathGenerator(Parameters(volatility, DayCountConvention::Act252)).Along(BusinessDays(On(2015, 1, 2), 2520), rng);
    const auto recovered = MarketParameters::From(path, Rate::Zero(), OrdinaryLeastSquaresFit());

    ASSERT_TRUE(recovered.has_value());
    EXPECT_NEAR(recovered->Volatility().Annualized(), volatility, 0.01);
}
