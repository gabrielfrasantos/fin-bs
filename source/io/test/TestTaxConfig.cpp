#include "source/io/TaxConfig.hpp"
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
    using fin::io::TaxConfig;
    using ::testing::HasSubstr;

    const std::array<std::pair<std::string_view, std::string_view>, 7> publishedFields{ { { "currency", R"("EUR")" },
        { "tax_year", "2025" },
        { "tax_free_allowance_cents", "5768400" },
        { "deposit_return_basis_points", "137" },
        { "investment_return_basis_points", "588" },
        { "tax_basis_points", "3600" },
        { "dividend_withholding_basis_points", "1500" } } };

    std::vector<std::string> Lines(const std::string& path)
    {
        std::ifstream file(path);
        std::vector<std::string> lines;

        for (std::string line; std::getline(file, line);)
            lines.push_back(line);

        return lines;
    }

    std::vector<std::string> PublishedRules()
    {
        return Lines(std::string(FIN_BS_CONFIG_DIRECTORY) + "/tax/nl_2025.json");
    }

    std::vector<std::string> RulesWith(std::string_view key, std::optional<std::string_view> value)
    {
        std::string document(R"({ "authority": "Belastingdienst")");

        for (const auto& [name, published] : publishedFields)
        {
            if (name != key)
                document += ", \"" + std::string(name) + "\": " + std::string(published);
            else if (value)
                document += ", \"" + std::string(name) + "\": " + std::string(*value);
        }

        return { document + " }" };
    }

    std::vector<std::string> RulesWithout(std::string_view key)
    {
        return RulesWith(key, std::nullopt);
    }
}

TEST(TestTaxConfig, the_published_rules_for_the_tax_year_parse)
{
    const TaxConfig config(PublishedRules());

    ASSERT_TRUE(config.Failure().empty()) << config.Failure();
    ASSERT_TRUE(config.Parameters().has_value());

    EXPECT_EQ(config.Parameters()->taxYear, 2025);
    EXPECT_EQ(config.Parameters()->taxFreeAllowance, Money::FromCents(5768400, Currency::Eur));
    EXPECT_EQ(config.Parameters()->depositReturnBasisPoints, 137);
    EXPECT_EQ(config.Parameters()->investmentReturnBasisPoints, 588);
    EXPECT_EQ(config.Parameters()->taxBasisPoints, 3600);
    EXPECT_EQ(config.Parameters()->dividendWithholdingBasisPoints, 1500);
}

TEST(TestTaxConfig, the_published_rules_are_well_formed)
{
    const TaxConfig config(PublishedRules());

    ASSERT_TRUE(config.Parameters().has_value());
    EXPECT_TRUE(IsWellFormed(*config.Parameters()));
}

TEST(TestTaxConfig, a_document_that_is_not_json_yields_no_rules)
{
    const TaxConfig config({ "{ this is not a set of tax rules" });

    EXPECT_THAT(config.Failure(), HasSubstr("Invalid JSON object"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, a_missing_percentage_names_the_key_that_is_missing)
{
    const TaxConfig config(RulesWithout("investment_return_basis_points"));

    EXPECT_THAT(config.Failure(), HasSubstr("investment_return_basis_points"));
    EXPECT_THAT(config.Failure(), HasSubstr("not found"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, a_missing_tax_year_names_the_key_that_is_missing)
{
    const TaxConfig config(RulesWithout("tax_year"));

    EXPECT_THAT(config.Failure(), HasSubstr("tax_year"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, a_negative_allowance_names_the_key_that_is_negative)
{
    const TaxConfig config(RulesWith("tax_free_allowance_cents", "-5768400"));

    EXPECT_THAT(config.Failure(), HasSubstr("tax_free_allowance_cents"));
    EXPECT_THAT(config.Failure(), HasSubstr("negative"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, a_percentage_beyond_the_whole_amount_is_refused)
{
    const TaxConfig config(RulesWith("tax_basis_points", "10001"));

    EXPECT_THAT(config.Failure(), HasSubstr("tax_basis_points"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, a_year_before_the_common_era_is_refused)
{
    const TaxConfig config(RulesWith("tax_year", "0"));

    EXPECT_THAT(config.Failure(), HasSubstr("tax_year"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, a_currency_the_project_does_not_hold_is_refused)
{
    const TaxConfig config(RulesWith("currency", R"("CHF")"));

    EXPECT_THAT(config.Failure(), HasSubstr("CHF"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, an_allowance_written_with_a_fraction_of_a_cent_names_the_key)
{
    const TaxConfig config(RulesWith("tax_free_allowance_cents", "5768400.5"));

    EXPECT_THAT(config.Failure(), HasSubstr("tax_free_allowance_cents"));
    EXPECT_FALSE(config.Parameters().has_value());
}

TEST(TestTaxConfig, rules_of_a_different_year_are_read_from_a_different_document)
{
    const TaxConfig config(RulesWith("tax_year", "2024"));

    ASSERT_TRUE(config.Parameters().has_value());
    EXPECT_EQ(config.Parameters()->taxYear, 2024);
}
