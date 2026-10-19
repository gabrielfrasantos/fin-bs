#include "source/io/BrokerConfig.hpp"
#include <array>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::io::BrokerConfig;
    using ::testing::HasSubstr;

    const std::array<std::pair<std::string_view, std::string_view>, 8> publishedFields{ { { "currency", R"("EUR")" },
        { "option_fee_per_contract_cents", "75" },
        { "share_commission_per_trade_cents", "200" },
        { "handling_fee_per_trade_cents", "100" },
        { "connectivity_fee_cap_per_exchange_per_year_cents", "250" },
        { "connectivity_basis_points", "25" },
        { "currency_handling_basis_points", "25" },
        { "debit_interest_basis_points", "690" } } };

    std::vector<std::string> Lines(const std::string& path)
    {
        std::ifstream file(path);
        std::vector<std::string> lines;

        for (std::string line; std::getline(file, line);)
            lines.push_back(line);

        return lines;
    }

    std::vector<std::string> PublishedSchedule()
    {
        return Lines(std::string(FIN_BS_CONFIG_DIRECTORY) + "/broker/degiro.json");
    }

    std::vector<std::string> ScheduleWith(std::string_view key, std::optional<std::string_view> value)
    {
        std::string document(R"({ "broker": "DEGIRO")");

        for (const auto& [name, published] : publishedFields)
        {
            if (name != key)
                document += ", \"" + std::string(name) + "\": " + std::string(published);
            else if (value)
                document += ", \"" + std::string(name) + "\": " + std::string(*value);
        }

        return { document + " }" };
    }

    std::vector<std::string> ScheduleWithout(std::string_view key)
    {
        return ScheduleWith(key, std::nullopt);
    }

    std::vector<std::string> AsOneLine(const std::vector<std::string>& lines)
    {
        std::string joined;

        for (const auto& line : lines)
            joined += line;

        return { joined };
    }
}

TEST(TestBrokerConfig, the_published_schedule_parses)
{
    const BrokerConfig config(PublishedSchedule());

    ASSERT_TRUE(config.Failure().empty()) << config.Failure();
    ASSERT_TRUE(config.Schedule().has_value());

    EXPECT_EQ(config.Schedule()->optionFeePerContract, Money::FromCents(75, Currency::Eur));
    EXPECT_EQ(config.Schedule()->shareCommissionPerTrade, Money::FromCents(200, Currency::Eur));
    EXPECT_EQ(config.Schedule()->handlingFeePerTrade, Money::FromCents(100, Currency::Eur));
    EXPECT_EQ(config.Schedule()->connectivityFeeCapPerExchangePerYear, Money::FromCents(250, Currency::Eur));
    EXPECT_EQ(config.Schedule()->connectivityBasisPoints, 25);
    EXPECT_EQ(config.Schedule()->currencyHandlingBasisPoints, 25);
    EXPECT_EQ(config.Schedule()->debitInterestBasisPoints, 690);
}

TEST(TestBrokerConfig, the_published_schedule_is_well_formed)
{
    const BrokerConfig config(PublishedSchedule());

    ASSERT_TRUE(config.Schedule().has_value());
    EXPECT_TRUE(IsWellFormed(*config.Schedule()));
}

TEST(TestBrokerConfig, a_document_that_is_not_json_yields_no_schedule)
{
    const BrokerConfig config({ "{ this is not a fee schedule" });

    EXPECT_THAT(config.Failure(), HasSubstr("Invalid JSON object"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, an_empty_document_yields_no_schedule)
{
    const BrokerConfig config({});

    EXPECT_THAT(config.Failure(), HasSubstr("Missing JSON data"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_missing_fee_names_the_key_that_is_missing)
{
    const BrokerConfig config(ScheduleWithout("handling_fee_per_trade_cents"));

    EXPECT_THAT(config.Failure(), HasSubstr("handling_fee_per_trade_cents"));
    EXPECT_THAT(config.Failure(), HasSubstr("not found"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_missing_currency_names_the_key_that_is_missing)
{
    const BrokerConfig config(ScheduleWithout("currency"));

    EXPECT_THAT(config.Failure(), HasSubstr("currency"));
    EXPECT_THAT(config.Failure(), HasSubstr("not found"));
}

TEST(TestBrokerConfig, a_negative_fee_names_the_key_that_is_negative)
{
    const BrokerConfig config(ScheduleWith("option_fee_per_contract_cents", "-75"));

    EXPECT_THAT(config.Failure(), HasSubstr("option_fee_per_contract_cents"));
    EXPECT_THAT(config.Failure(), HasSubstr("negative"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_proportional_cost_beyond_the_whole_amount_is_refused)
{
    const BrokerConfig config(ScheduleWith("currency_handling_basis_points", "10001"));

    EXPECT_THAT(config.Failure(), HasSubstr("currency_handling_basis_points"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_currency_the_project_does_not_hold_is_refused)
{
    const BrokerConfig config(ScheduleWith("currency", R"("CHF")"));

    EXPECT_THAT(config.Failure(), HasSubstr("CHF"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_fee_written_as_a_word_names_the_key_and_the_kind_expected)
{
    const BrokerConfig config(ScheduleWith("handling_fee_per_trade_cents", R"("one euro")"));

    EXPECT_THAT(config.Failure(), HasSubstr("Integer"));
    EXPECT_THAT(config.Failure(), HasSubstr("handling_fee_per_trade_cents"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_fee_written_with_a_fraction_of_a_cent_names_the_key)
{
    const BrokerConfig config(ScheduleWith("option_fee_per_contract_cents", "0.75"));

    EXPECT_THAT(config.Failure(), HasSubstr("option_fee_per_contract_cents"));
    EXPECT_FALSE(config.Schedule().has_value());
}

TEST(TestBrokerConfig, a_schedule_quoted_in_another_currency_is_read_in_that_currency)
{
    const BrokerConfig config(ScheduleWith("currency", R"("USD")"));

    ASSERT_TRUE(config.Schedule().has_value());
    EXPECT_EQ(config.Schedule()->optionFeePerContract, Money::FromCents(75, Currency::Usd));
}

TEST(TestBrokerConfig, a_schedule_split_over_many_lines_reads_the_same_as_one_line)
{
    const BrokerConfig split(PublishedSchedule());
    const BrokerConfig single(AsOneLine(PublishedSchedule()));

    ASSERT_TRUE(split.Schedule().has_value());
    ASSERT_TRUE(single.Schedule().has_value());
    EXPECT_EQ(split.Schedule()->optionFeePerContract, single.Schedule()->optionFeePerContract);
}
