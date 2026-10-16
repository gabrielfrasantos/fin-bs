#include "source/io/PriceCsvLoader.hpp"
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Money;
    using fin::io::PriceCsvLoader;
    using ::testing::HasSubstr;

    std::vector<std::string> Lines(const std::string& path)
    {
        std::ifstream file(path);
        std::vector<std::string> lines;

        for (std::string line; std::getline(file, line);)
            lines.push_back(line);

        return lines;
    }

    std::vector<std::string> PublishedCloses()
    {
        return Lines(std::string(FIN_BS_DATA_DIRECTORY) + "/prices/example.csv");
    }

    std::vector<std::string> ClosedAt(const std::vector<std::string>& rows)
    {
        std::vector<std::string> document{ "date,close" };
        document.insert(document.end(), rows.begin(), rows.end());

        return document;
    }

    PriceCsvLoader LoadedFrom(const std::vector<std::string>& document)
    {
        return PriceCsvLoader(document, Currency::Eur, DayCountConvention::Act252);
    }

    TEST(TestPriceCsvLoader, reads_the_published_closes)
    {
        const PriceCsvLoader loader(PublishedCloses(), Currency::Eur, DayCountConvention::Act252);

        ASSERT_TRUE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), testing::IsEmpty());
        EXPECT_EQ(loader.Series()->Size(), 22u);
        EXPECT_EQ(loader.Series()->GetCurrency(), Currency::Eur);
        EXPECT_EQ(loader.Series()->Convention(), DayCountConvention::Act252);
        EXPECT_EQ(loader.Series()->Dates().front(), Date::FromYearMonthDay(2025, 1, 2));
        EXPECT_EQ(loader.Series()->Dates().back(), Date::FromYearMonthDay(2025, 1, 31));
        EXPECT_EQ(loader.Series()->Closes().front(), Money::FromCents(9500, Currency::Eur));
        EXPECT_EQ(loader.Series()->Closes().back(), Money::FromCents(10086, Currency::Eur));
    }

    TEST(TestPriceCsvLoader, reads_the_currency_and_the_day_count_it_is_given)
    {
        const PriceCsvLoader loader(PublishedCloses(), Currency::Usd, DayCountConvention::Act365Fixed);

        ASSERT_TRUE(loader.Series().has_value());
        EXPECT_EQ(loader.Series()->GetCurrency(), Currency::Usd);
        EXPECT_EQ(loader.Series()->Convention(), DayCountConvention::Act365Fixed);
    }

    TEST(TestPriceCsvLoader, tolerates_rows_ended_by_a_carriage_return)
    {
        std::vector<std::string> document;

        for (const auto& line : PublishedCloses())
            document.push_back(line + "\r");

        const auto ended = LoadedFrom(document);
        const auto plain = LoadedFrom(PublishedCloses());

        ASSERT_TRUE(ended.Series().has_value());
        EXPECT_EQ(ended.Series()->Size(), plain.Series()->Size());
        EXPECT_EQ(ended.Series()->Closes().back(), plain.Series()->Closes().back());
    }

    TEST(TestPriceCsvLoader, tolerates_blank_rows)
    {
        const auto loader = LoadedFrom({ "", "date,close", "2025-01-02,95.00", "", "2025-01-03,95.62", "" });

        ASSERT_TRUE(loader.Series().has_value());
        EXPECT_EQ(loader.Series()->Size(), 2u);
    }

    TEST(TestPriceCsvLoader, refuses_a_file_that_names_other_columns)
    {
        const auto loader = LoadedFrom({ "date,open,close", "2025-01-02,94.00,95.00" });

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("date,open,close"));
        EXPECT_THAT(loader.Failure(), HasSubstr("Line 1"));
    }

    TEST(TestPriceCsvLoader, refuses_a_file_that_names_no_columns)
    {
        const auto loader = LoadedFrom({});

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("names no columns"));
    }

    TEST(TestPriceCsvLoader, refuses_a_row_that_is_not_a_date_and_a_close)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95.00", "2025-01-03" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("Line 3"));
        EXPECT_THAT(loader.Failure(), HasSubstr("2025-01-03"));
    }

    TEST(TestPriceCsvLoader, refuses_dates_that_are_out_of_order)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95.00", "2025-01-06,96.18", "2025-01-03,95.62" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("Line 4"));
        EXPECT_THAT(loader.Failure(), HasSubstr("2025-01-03"));
        EXPECT_THAT(loader.Failure(), HasSubstr("comes before"));
    }

    TEST(TestPriceCsvLoader, refuses_a_date_that_is_closed_twice)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95.00", "2025-01-02,95.62" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("Line 3"));
        EXPECT_THAT(loader.Failure(), HasSubstr("closed twice"));
    }

    TEST(TestPriceCsvLoader, refuses_a_day_that_is_not_on_the_calendar)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-02-30,95.00", "2025-03-03,95.62" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("2025-02-30"));
        EXPECT_THAT(loader.Failure(), HasSubstr("not a day of the calendar"));
    }

    TEST(TestPriceCsvLoader, refuses_a_date_that_is_not_written_as_one)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2 January 2025,95.00", "2025-01-03,95.62" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("2 January 2025"));
    }

    TEST(TestPriceCsvLoader, refuses_a_close_that_is_not_an_amount)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,ninety five", "2025-01-03,95.62" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("ninety five"));
    }

    TEST(TestPriceCsvLoader, refuses_a_close_that_is_not_positive)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,0.00", "2025-01-03,95.62" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("not a positive amount"));
    }

    TEST(TestPriceCsvLoader, refuses_a_file_holding_one_close)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95.00" }));

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("at least two closes"));
    }

    TEST(TestPriceCsvLoader, refuses_closes_that_span_no_time_under_the_day_count_they_are_read_with)
    {
        const PriceCsvLoader loader(ClosedAt({ "2025-01-04,95.00", "2025-01-05,95.62" }), Currency::Eur, DayCountConvention::Act252);

        EXPECT_FALSE(loader.Series().has_value());
        EXPECT_THAT(loader.Failure(), HasSubstr("span no time"));
    }

    TEST(TestPriceCsvLoader, reads_a_close_written_without_a_fraction)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95", "2025-01-03,95.6" }));

        ASSERT_TRUE(loader.Series().has_value());
        EXPECT_EQ(loader.Series()->Closes().front(), Money::FromCents(9500, Currency::Eur));
        EXPECT_EQ(loader.Series()->Closes().back(), Money::FromCents(9560, Currency::Eur));
    }

    TEST(TestPriceCsvLoader, rounds_a_close_written_beyond_the_cent_to_the_nearest_cent)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95.004999", "2025-01-03,95.005001" }));

        ASSERT_TRUE(loader.Series().has_value());
        EXPECT_EQ(loader.Series()->Closes().front(), Money::FromCents(9500, Currency::Eur));
        EXPECT_EQ(loader.Series()->Closes().back(), Money::FromCents(9501, Currency::Eur));
    }

    TEST(TestPriceCsvLoader, settles_a_close_halfway_between_two_cents_on_the_even_one)
    {
        const auto loader = LoadedFrom(ClosedAt({ "2025-01-02,95.005", "2025-01-03,95.015" }));

        ASSERT_TRUE(loader.Series().has_value());
        EXPECT_EQ(loader.Series()->Closes().front(), Money::FromCents(9500, Currency::Eur));
        EXPECT_EQ(loader.Series()->Closes().back(), Money::FromCents(9502, Currency::Eur));
    }
}
