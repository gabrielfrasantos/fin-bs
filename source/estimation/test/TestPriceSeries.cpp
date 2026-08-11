#include "source/estimation/PriceSeries.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::estimation::PriceSeries;

    std::vector<Date> BusinessDays(std::size_t count, const Date& start)
    {
        std::vector<Date> result;
        auto date = start;

        while (result.size() != count)
        {
            if (!date.IsWeekend())
                result.push_back(date);

            date = date.AddDays(1);
        }

        return result;
    }

    std::vector<Money> ConstantCloses(std::size_t count, double amount)
    {
        return std::vector<Money>(count, Money::FromAmount(amount, Currency::Eur));
    }

    class TestPriceSeries
        : public ::testing::Test
    {
    protected:
        const Date monday{ Date::FromYearMonthDay(2024, 1, 1) };
    };
}

TEST_F(TestPriceSeries, a_series_reports_its_size_currency_and_convention)
{
    const auto series = PriceSeries::From(BusinessDays(5, monday), ConstantCloses(5, 100.0), DayCountConvention::Act252);

    EXPECT_EQ(series.Size(), 5u);
    EXPECT_EQ(series.GetCurrency(), Currency::Eur);
    EXPECT_EQ(series.Convention(), DayCountConvention::Act252);
}

TEST_F(TestPriceSeries, consecutive_business_days_are_one_two_hundred_and_fifty_secondth_of_a_year_apart)
{
    const auto series = PriceSeries::From(BusinessDays(11, monday), ConstantCloses(11, 100.0), DayCountConvention::Act252);
    const auto times = series.Times();

    EXPECT_EQ(times.front(), 0.0);

    for (std::size_t i = 0; i != times.size(); ++i)
        EXPECT_DOUBLE_EQ(times[i], static_cast<double>(i) / 252.0);

    EXPECT_DOUBLE_EQ(series.Horizon(), 10.0 / 252.0);
    EXPECT_DOUBLE_EQ(series.PeriodsPerYear(), 252.0);
}

TEST_F(TestPriceSeries, calendar_days_are_annualised_by_the_fixed_year_length)
{
    std::vector<Date> dates{ monday, monday.AddDays(365) };

    const auto series = PriceSeries::From(dates, ConstantCloses(2, 100.0), DayCountConvention::Act365Fixed);

    EXPECT_DOUBLE_EQ(series.Horizon(), 1.0);
    EXPECT_DOUBLE_EQ(series.PeriodsPerYear(), 1.0);
}

TEST_F(TestPriceSeries, a_series_exposes_the_dates_and_closes_it_was_built_from)
{
    const auto dates = BusinessDays(3, monday);
    const auto closes = ConstantCloses(3, 12.34);
    const auto series = PriceSeries::From(dates, closes, DayCountConvention::Act252);

    EXPECT_TRUE(std::equal(series.Dates().begin(), series.Dates().end(), dates.begin()));
    EXPECT_TRUE(std::equal(series.Closes().begin(), series.Closes().end(), closes.begin()));
}

TEST_F(TestPriceSeries, log_returns_of_a_series_are_one_shorter_than_its_closes)
{
    const auto series = PriceSeries::From(BusinessDays(4, monday), ConstantCloses(4, 100.0), DayCountConvention::Act252);

    EXPECT_EQ(series.LogPrices().size(), 4u);
    EXPECT_EQ(series.LogReturns().size(), 3u);
}

TEST_F(TestPriceSeries, a_series_of_fewer_than_two_observations_stops_the_run)
{
    EXPECT_DEATH(PriceSeries::From(BusinessDays(1, monday), ConstantCloses(1, 100.0), DayCountConvention::Act252), "");
    EXPECT_DEATH(PriceSeries::From({}, {}, DayCountConvention::Act252), "");
}

TEST_F(TestPriceSeries, mismatched_dates_and_closes_stop_the_run)
{
    EXPECT_DEATH(PriceSeries::From(BusinessDays(3, monday), ConstantCloses(2, 100.0), DayCountConvention::Act252), "");
}

TEST_F(TestPriceSeries, dates_that_do_not_strictly_increase_stop_the_run)
{
    const std::vector<Date> duplicated{ monday, monday };
    const std::vector<Date> reversed{ monday.AddDays(1), monday };

    EXPECT_DEATH(PriceSeries::From(duplicated, ConstantCloses(2, 100.0), DayCountConvention::Act252), "");
    EXPECT_DEATH(PriceSeries::From(reversed, ConstantCloses(2, 100.0), DayCountConvention::Act252), "");
}

TEST_F(TestPriceSeries, a_close_that_is_not_positive_stops_the_run)
{
    std::vector<Money> closes{ Money::FromAmount(100.0, Currency::Eur), Money::Zero(Currency::Eur) };

    EXPECT_DEATH(PriceSeries::From(BusinessDays(2, monday), closes, DayCountConvention::Act252), "");
}

TEST_F(TestPriceSeries, a_series_that_spans_no_time_at_all_stops_the_run)
{
    const auto saturday = Date::FromYearMonthDay(2024, 1, 6);
    const std::vector<Date> weekend{ saturday, saturday.AddDays(1) };

    EXPECT_DEATH(PriceSeries::From(weekend, ConstantCloses(2, 100.0), DayCountConvention::Act252), "");
}
