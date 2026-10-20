#include "source/sim/HistoricalPath.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::estimation::PriceSeries;
    using fin::sim::HistoricalPath;
    using ::testing::ElementsAre;
    using ::testing::HasSubstr;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    PriceSeries AWeekOfCloses()
    {
        return PriceSeries::From({ On(2025, 1, 6), On(2025, 1, 7), On(2025, 1, 8), On(2025, 1, 9), On(2025, 1, 10), On(2025, 1, 13) },
            { Euro(100.00), Euro(101.00), Euro(102.00), Euro(103.00), Euro(104.00), Euro(105.00) },
            DayCountConvention::Act252);
    }
}

TEST(TestHistoricalPath, replays_the_close_of_every_scheduled_date)
{
    const std::vector<Date> schedule{ On(2025, 1, 6), On(2025, 1, 8), On(2025, 1, 13) };
    const HistoricalPath replay(AWeekOfCloses(), schedule);

    ASSERT_TRUE(replay.Path().has_value());
    EXPECT_THAT(replay.Path()->Dates(), ElementsAre(On(2025, 1, 6), On(2025, 1, 8), On(2025, 1, 13)));
    EXPECT_THAT(replay.Path()->Closes(), ElementsAre(Euro(100.00), Euro(102.00), Euro(105.00)));
}

TEST(TestHistoricalPath, carries_the_last_close_forward_over_a_day_the_market_was_shut)
{
    const std::vector<Date> schedule{ On(2025, 1, 7), On(2025, 1, 11), On(2025, 1, 12) };
    const HistoricalPath replay(AWeekOfCloses(), schedule);

    ASSERT_TRUE(replay.Path().has_value());
    EXPECT_THAT(replay.Path()->Closes(), ElementsAre(Euro(101.00), Euro(104.00), Euro(104.00)));
}

TEST(TestHistoricalPath, refuses_a_schedule_that_opens_before_the_history_begins)
{
    const std::vector<Date> schedule{ On(2025, 1, 3), On(2025, 1, 10) };
    const HistoricalPath replay(AWeekOfCloses(), schedule);

    EXPECT_FALSE(replay.Path().has_value());
    EXPECT_THAT(replay.Failure(), HasSubstr("begins on 2025-01-06"));
    EXPECT_THAT(replay.Failure(), HasSubstr("2025-01-03"));
}

TEST(TestHistoricalPath, refuses_a_schedule_that_reaches_past_the_history)
{
    const std::vector<Date> schedule{ On(2025, 1, 6), On(2025, 1, 14) };
    const HistoricalPath replay(AWeekOfCloses(), schedule);

    EXPECT_FALSE(replay.Path().has_value());
    EXPECT_THAT(replay.Failure(), HasSubstr("ends on 2025-01-13"));
    EXPECT_THAT(replay.Failure(), HasSubstr("2025-01-14"));
}
