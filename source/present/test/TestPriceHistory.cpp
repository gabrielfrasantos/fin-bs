#include "source/present/PriceHistory.hpp"
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::present::PriceHistory;
    using ::testing::HasSubstr;

    std::vector<std::string> PublishedCloses()
    {
        std::ifstream file(std::string(FIN_BS_DATA_DIRECTORY) + "/prices/example.csv");
        std::vector<std::string> lines;

        for (std::string line; std::getline(file, line);)
            lines.push_back(line);

        return lines;
    }

    TEST(TestPriceHistory, estimates_a_volatility_and_a_drift_from_a_published_history)
    {
        const PriceHistory history(PublishedCloses(), Currency::Eur, 180);

        ASSERT_TRUE(history.Estimate().has_value());
        EXPECT_TRUE(history.Failure().empty());
        EXPECT_GT(history.Estimate()->volatilityBasisPoints, 0);
    }

    TEST(TestPriceHistory, reports_a_history_it_cannot_read_rather_than_guessing_at_one)
    {
        const PriceHistory history({ "date,close", "2025-01-02,not a price" }, Currency::Eur, 0);

        EXPECT_FALSE(history.Estimate().has_value());
        EXPECT_THAT(history.Failure(), HasSubstr("Line"));
    }

    TEST(TestPriceHistory, reports_a_history_that_never_moves_rather_than_calling_it_riskless)
    {
        const PriceHistory history({ "date,close", "2025-01-02,10.00", "2025-01-03,10.00", "2025-01-06,10.00" }, Currency::Eur, 0);

        EXPECT_FALSE(history.Estimate().has_value());
        EXPECT_THAT(history.Failure(), HasSubstr("does not move enough"));
    }
}
