#include "source/sim/BookScript.hpp"
#include "source/sim/GbmPathGenerator.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::DayCountConvention;
    using fin::core::Delta;
    using fin::core::Money;
    using fin::core::Rate;
    using fin::core::Volatility;
    using fin::costs::BrokerFees;
    using fin::costs::FeeSchedule;
    using fin::sim::AnnualReference;
    using fin::sim::BookScript;
    using fin::sim::BookTerms;
    using fin::sim::GbmParameters;
    using fin::sim::GbmPathGenerator;
    using fin::sim::IsWellFormed;
    using fin::sim::Rng;
    using fin::sim::UnderlyingTerms;
    using fin::strategy::Book;
    using fin::strategy::Portfolio;
    using fin::strategy::ReinvestmentPolicy;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;
    using fin::tax::Box3Parameters;
    using fin::tax::Box3TaxModel;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    UnderlyingTerms Market(std::string underlying, double initialPrice)
    {
        return { .underlying = std::move(underlying),
            .initialPrice = Euro(initialPrice),
            .drift = Rate::FromContinuous(0.07),
            .volatility = Volatility::FromAnnualized(0.25),
            .dividendYield = Rate::Zero(),
            .strikeIncrement = Euro(1.00) };
    }

    BookTerms Terms(std::vector<UnderlyingTerms> underlyings)
    {
        return { .start = On(2025, 1, 2),
            .cycles = 6,
            .daysPerCycle = 182,
            .observationDaysBeforeExpiry = 30,
            .targetDelta = Delta::FromValue(0.30),
            .riskFreeRate = Rate::FromContinuous(0.02),
            .assessment = AnnualReference{ .month = 1, .day = 1 },
            .underlyings = std::move(underlyings) };
    }

    Book Of(const BookScript& script)
    {
        return Book(script.Underlyings(),
            Portfolio::Funded(Euro(200000.00)),
            ReinvestmentPolicy::WithCashBuffer(500),
            RollRules(RollTriggers{ .daysToExpiry = 7, .profitCaptureBasisPoints = 8000, .deltaLimit = Delta::FromValue(0.60) }),
            BrokerFees(FeeSchedule{ .optionFeePerContract = Euro(0.75),
                .shareCommissionPerTrade = Euro(2.00),
                .handlingFeePerTrade = Euro(1.00),
                .connectivityFeeCapPerExchangePerYear = Euro(2.50),
                .connectivityBasisPoints = 25,
                .currencyHandlingBasisPoints = 25 }),
            Box3TaxModel(Box3Parameters{ .taxYear = 2025,
                .taxFreeAllowance = Euro(57684.00),
                .depositReturnBasisPoints = 137,
                .investmentReturnBasisPoints = 588,
                .taxBasisPoints = 3600,
                .dividendWithholdingBasisPoints = 1500 }));
    }

    std::vector<fin::estimation::PriceSeries> Paths(const BookTerms& terms, const BookScript& script, Rng& rng)
    {
        std::vector<fin::estimation::PriceSeries> paths;

        for (const auto& underlying : terms.underlyings)
            paths.push_back(GbmPathGenerator(GbmParameters{ .initialPrice = underlying.initialPrice,
                                                 .drift = underlying.drift,
                                                 .volatility = underlying.volatility,
                                                 .convention = DayCountConvention::Act365Fixed })
                    .Along(script.Schedule(), rng));

        return paths;
    }

    class TestBookScript
        : public ::testing::Test
    {
    protected:
        BookTerms terms{ Terms({ Market("ACME", 40.00), Market("BETA", 55.00) }) };
        BookScript script{ terms };
    };
}

TEST_F(TestBookScript, a_script_names_its_underlyings_in_the_order_it_was_given_them)
{
    EXPECT_EQ(script.Underlyings(), (std::vector<std::string>{ "ACME", "BETA" }));
}

TEST_F(TestBookScript, the_schedule_is_one_set_of_dates_shared_by_every_underlying)
{
    const auto dates = script.Schedule();
    auto deduplicated = dates;

    deduplicated.erase(std::unique(deduplicated.begin(), deduplicated.end()), deduplicated.end());

    EXPECT_EQ(dates.front(), On(2025, 1, 2));
    EXPECT_TRUE(std::is_sorted(dates.begin(), dates.end()));
    EXPECT_EQ(deduplicated.size(), dates.size());
}

TEST_F(TestBookScript, the_schedule_carries_every_reference_date_the_book_will_be_assessed_on)
{
    const auto dates = script.Schedule();

    EXPECT_NE(std::find(dates.begin(), dates.end(), On(2026, 1, 1)), dates.end());
    EXPECT_NE(std::find(dates.begin(), dates.end(), On(2027, 1, 1)), dates.end());
}

TEST_F(TestBookScript, a_cycle_carries_one_period_for_every_underlying_in_order)
{
    Rng rng(20250811u);
    auto book = Of(script);
    const auto paths = Paths(terms, script, rng);
    const auto cycle = script.CycleFor(0, book, paths);

    ASSERT_EQ(cycle.size(), 2u);
    EXPECT_EQ(cycle[0].underlying, "ACME");
    EXPECT_EQ(cycle[1].underlying, "BETA");
    EXPECT_EQ(cycle[0].period.opened, On(2025, 1, 2));
    EXPECT_EQ(cycle[1].period.opened, On(2025, 1, 2));
    EXPECT_EQ(cycle[0].period.expiry, cycle[1].period.expiry);
}

TEST_F(TestBookScript, a_period_written_for_a_book_never_carries_an_assessment_of_its_own)
{
    Rng rng(20250811u);
    auto book = Of(script);
    const auto paths = Paths(terms, script, rng);

    for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
        for (const auto& entry : script.CycleFor(cycle, book, paths))
            EXPECT_FALSE(entry.period.assessed.has_value());
}

TEST_F(TestBookScript, the_book_is_assessed_once_a_year_and_priced_on_the_reference_date)
{
    Rng rng(20250811u);
    const auto paths = Paths(terms, script, rng);

    std::vector<Date> struck;

    for (std::int32_t cycle = 0; cycle != script.Cycles(); ++cycle)
        if (const auto assessed = script.AssessmentBefore(cycle, paths))
        {
            struck.push_back(assessed->on);

            EXPECT_EQ(assessed->prices.size(), 2u);
            EXPECT_EQ(assessed->prices[0].underlying, "ACME");
            EXPECT_EQ(assessed->prices[1].underlying, "BETA");
        }

    EXPECT_EQ(struck, (std::vector<Date>{ On(2026, 1, 1), On(2027, 1, 1) }));
}

TEST_F(TestBookScript, a_book_of_underlyings_that_are_not_all_named_apart_is_refused)
{
    EXPECT_FALSE(IsWellFormed(Terms({})));
    EXPECT_FALSE(IsWellFormed(Terms({ Market("ACME", 40.00), Market("ACME", 55.00) })));
    EXPECT_FALSE(IsWellFormed(Terms({ Market("", 40.00) })));
    EXPECT_FALSE(IsWellFormed(Terms({ Market("ACME", 0.00) })));
    EXPECT_TRUE(IsWellFormed(Terms({ Market("ACME", 40.00), Market("BETA", 55.00) })));
}
