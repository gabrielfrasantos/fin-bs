#include "source/core/Date.hpp"
#include "source/core/DayCount.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::BusinessDaysBetween;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::YearFraction;

    class TestDayCount
        : public ::testing::Test
    {
    protected:
        static constexpr double epsilon = 1e-9;
    };
}

TEST_F(TestDayCount, act365_fixed_keeps_a_fixed_denominator_across_a_leap_year)
{
    EXPECT_NEAR(YearFraction(Date::FromYearMonthDay(2024, 1, 1), Date::FromYearMonthDay(2025, 1, 1), DayCountConvention::Act365Fixed), 366.0 / 365.0, epsilon);
    EXPECT_NEAR(YearFraction(Date::FromYearMonthDay(2023, 1, 1), Date::FromYearMonthDay(2024, 1, 1), DayCountConvention::Act365Fixed), 1.0, epsilon);
    EXPECT_NEAR(YearFraction(Date::FromYearMonthDay(2025, 1, 17), Date::FromYearMonthDay(2025, 2, 21), DayCountConvention::Act365Fixed), 35.0 / 365.0, epsilon);
}

TEST_F(TestDayCount, a_period_of_zero_length_has_a_zero_year_fraction)
{
    const auto date = Date::FromYearMonthDay(2025, 3, 21);

    EXPECT_NEAR(YearFraction(date, date, DayCountConvention::Act365Fixed), 0.0, epsilon);
    EXPECT_NEAR(YearFraction(date, date, DayCountConvention::Act252), 0.0, epsilon);
}

TEST_F(TestDayCount, counts_business_days_excluding_weekends)
{
    EXPECT_EQ(BusinessDaysBetween(Date::FromYearMonthDay(2025, 6, 20), Date::FromYearMonthDay(2025, 6, 23)), 1);
    EXPECT_EQ(BusinessDaysBetween(Date::FromYearMonthDay(2025, 6, 23), Date::FromYearMonthDay(2025, 6, 30)), 5);
    EXPECT_EQ(BusinessDaysBetween(Date::FromYearMonthDay(2025, 1, 17), Date::FromYearMonthDay(2025, 2, 21)), 25);
    EXPECT_EQ(BusinessDaysBetween(Date::FromYearMonthDay(2024, 1, 1), Date::FromYearMonthDay(2025, 1, 1)), 262);
    EXPECT_EQ(BusinessDaysBetween(Date::FromYearMonthDay(2023, 1, 1), Date::FromYearMonthDay(2024, 1, 1)), 260);
}

TEST_F(TestDayCount, act252_counts_trading_time_not_calendar_time)
{
    EXPECT_NEAR(YearFraction(Date::FromYearMonthDay(2024, 1, 1), Date::FromYearMonthDay(2025, 1, 1), DayCountConvention::Act252), 262.0 / 252.0, epsilon);
    EXPECT_NEAR(YearFraction(Date::FromYearMonthDay(2025, 6, 20), Date::FromYearMonthDay(2025, 6, 23), DayCountConvention::Act252), 1.0 / 252.0, epsilon);
}

TEST_F(TestDayCount, a_weekend_advances_calendar_time_faster_than_trading_time)
{
    const auto friday = Date::FromYearMonthDay(2025, 6, 20);
    const auto monday = Date::FromYearMonthDay(2025, 6, 23);

    EXPECT_GT(YearFraction(friday, monday, DayCountConvention::Act365Fixed), YearFraction(friday, monday, DayCountConvention::Act252));
}

TEST_F(TestDayCount, is_usable_in_a_constant_expression)
{
    static_assert(BusinessDaysBetween(Date::FromYearMonthDay(2025, 6, 23), Date::FromYearMonthDay(2025, 6, 30)) == 5);
    static_assert(YearFraction(Date::FromYearMonthDay(2023, 1, 1), Date::FromYearMonthDay(2024, 1, 1), DayCountConvention::Act365Fixed) == 1.0);
}

TEST_F(TestDayCount, rejects_a_reversed_period)
{
    const auto early = Date::FromYearMonthDay(2025, 1, 1);
    const auto late = Date::FromYearMonthDay(2025, 2, 1);

    EXPECT_DEATH(static_cast<void>(YearFraction(late, early, DayCountConvention::Act365Fixed)), "");
    EXPECT_DEATH(static_cast<void>(BusinessDaysBetween(late, early)), "");
}
