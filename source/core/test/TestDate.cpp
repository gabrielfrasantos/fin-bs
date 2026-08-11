#include "source/core/Date.hpp"
#include <cstdint>
#include <gtest/gtest.h>

namespace
{
    using fin::core::Date;
    using fin::core::Weekday;

    class TestDate
        : public ::testing::Test
    {};
}

TEST_F(TestDate, epoch_is_the_first_of_january_nineteen_seventy)
{
    const auto epoch = Date::FromYearMonthDay(1970, 1, 1);

    EXPECT_EQ(epoch.DaysSinceEpoch(), 0);
    EXPECT_EQ(epoch.Year(), 1970);
    EXPECT_EQ(epoch.Month(), 1u);
    EXPECT_EQ(epoch.Day(), 1u);
    EXPECT_EQ(epoch.DayOfWeek(), Weekday::Thursday);
}

TEST_F(TestDate, is_usable_in_a_constant_expression)
{
    static_assert(Date::FromYearMonthDay(1970, 1, 1).DaysSinceEpoch() == 0);
    static_assert(Date::FromYearMonthDay(2025, 1, 1) - Date::FromYearMonthDay(2024, 1, 1) == 366);
    static_assert(Date::IsLeapYear(2000));
}

TEST_F(TestDate, round_trips_through_year_month_day_over_two_centuries)
{
    for (auto days = Date::FromYearMonthDay(1900, 1, 1).DaysSinceEpoch(); days < Date::FromYearMonthDay(2100, 1, 1).DaysSinceEpoch(); days += 13)
    {
        const auto date = Date::FromDaysSinceEpoch(days);
        const auto rebuilt = Date::FromYearMonthDay(date.Year(), date.Month(), date.Day());

        ASSERT_EQ(rebuilt.DaysSinceEpoch(), days);
    }
}

TEST_F(TestDate, applies_the_gregorian_leap_year_rule)
{
    EXPECT_FALSE(Date::IsLeapYear(1900));
    EXPECT_TRUE(Date::IsLeapYear(2000));
    EXPECT_TRUE(Date::IsLeapYear(2024));
    EXPECT_FALSE(Date::IsLeapYear(2025));
    EXPECT_FALSE(Date::IsLeapYear(2100));

    EXPECT_EQ(Date::DaysInMonth(1900, 2), 28u);
    EXPECT_EQ(Date::DaysInMonth(2000, 2), 29u);
    EXPECT_EQ(Date::DaysInMonth(2024, 2), 29u);
    EXPECT_EQ(Date::DaysInMonth(2025, 2), 28u);
    EXPECT_EQ(Date::DaysInMonth(2025, 12), 31u);
}

TEST_F(TestDate, counts_days_between_dates_including_the_leap_day)
{
    EXPECT_EQ(Date::FromYearMonthDay(2025, 1, 1) - Date::FromYearMonthDay(2024, 1, 1), 366);
    EXPECT_EQ(Date::FromYearMonthDay(2024, 1, 1) - Date::FromYearMonthDay(2023, 1, 1), 365);
    EXPECT_EQ(Date::FromYearMonthDay(2024, 1, 1) - Date::FromYearMonthDay(2024, 1, 1), 0);
    EXPECT_EQ(Date::FromYearMonthDay(2024, 1, 1) - Date::FromYearMonthDay(2025, 1, 1), -366);
}

TEST_F(TestDate, adds_days_across_month_and_year_boundaries)
{
    const auto endOfFebruary = Date::FromYearMonthDay(2024, 2, 28).AddDays(1);
    EXPECT_EQ(endOfFebruary.Month(), 2u);
    EXPECT_EQ(endOfFebruary.Day(), 29u);

    const auto newYear = Date::FromYearMonthDay(2024, 12, 31).AddDays(1);
    EXPECT_EQ(newYear.Year(), 2025);
    EXPECT_EQ(newYear.Month(), 1u);
    EXPECT_EQ(newYear.Day(), 1u);

    EXPECT_EQ(Date::FromYearMonthDay(2025, 3, 1).AddDays(-1), Date::FromYearMonthDay(2025, 2, 28));
}

TEST_F(TestDate, reports_the_weekday_of_expiry_fridays)
{
    EXPECT_EQ(Date::FromYearMonthDay(2025, 3, 21).DayOfWeek(), Weekday::Friday);
    EXPECT_EQ(Date::FromYearMonthDay(2025, 1, 17).DayOfWeek(), Weekday::Friday);
    EXPECT_EQ(Date::FromYearMonthDay(2024, 1, 1).DayOfWeek(), Weekday::Monday);
    EXPECT_EQ(Date::FromYearMonthDay(2023, 1, 1).DayOfWeek(), Weekday::Sunday);
    EXPECT_EQ(Date::FromYearMonthDay(1900, 1, 1).DayOfWeek(), Weekday::Monday);
}

TEST_F(TestDate, identifies_weekends)
{
    EXPECT_FALSE(Date::FromYearMonthDay(2025, 6, 20).IsWeekend());
    EXPECT_TRUE(Date::FromYearMonthDay(2025, 6, 21).IsWeekend());
    EXPECT_TRUE(Date::FromYearMonthDay(2025, 6, 22).IsWeekend());
    EXPECT_FALSE(Date::FromYearMonthDay(2025, 6, 23).IsWeekend());
}

TEST_F(TestDate, orders_chronologically)
{
    EXPECT_TRUE(Date::FromYearMonthDay(2025, 1, 1) < Date::FromYearMonthDay(2025, 1, 2));
    EXPECT_TRUE(Date::FromYearMonthDay(2025, 1, 31) < Date::FromYearMonthDay(2025, 2, 1));
    EXPECT_TRUE(Date::FromYearMonthDay(2024, 12, 31) < Date::FromYearMonthDay(2025, 1, 1));
    EXPECT_TRUE(Date::FromYearMonthDay(2025, 1, 1) == Date::FromYearMonthDay(2025, 1, 1));
}

TEST_F(TestDate, rejects_impossible_calendar_dates)
{
    EXPECT_FALSE(Date::IsValid(2025, 2, 29));
    EXPECT_TRUE(Date::IsValid(2024, 2, 29));
    EXPECT_FALSE(Date::IsValid(2025, 4, 31));
    EXPECT_FALSE(Date::IsValid(2025, 0, 1));

    EXPECT_DEATH(static_cast<void>(Date::FromYearMonthDay(2025, 2, 30)), "");
    EXPECT_DEATH(static_cast<void>(Date::FromYearMonthDay(2025, 13, 1)), "");
}
