#include "source/sim/BookMonteCarlo.hpp"
#include <gtest/gtest.h>
#include <span>
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
    using fin::sim::BookMonteCarlo;
    using fin::sim::BookScript;
    using fin::sim::BookTerms;
    using fin::sim::GbmParameters;
    using fin::sim::GbmPathGenerator;
    using fin::sim::Rng;
    using fin::sim::RunObserver;
    using fin::sim::UnderlyingTerms;
    using fin::strategy::Book;
    using fin::strategy::Portfolio;
    using fin::strategy::Reconciles;
    using fin::strategy::ReinvestmentPolicy;
    using fin::strategy::RollRules;
    using fin::strategy::RollTriggers;
    using fin::tax::Box3Parameters;
    using fin::tax::Box3TaxModel;

    constexpr std::uint64_t seed = 20250811u;
    constexpr auto capital = 200000.00;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    BrokerFees Fees()
    {
        return BrokerFees(FeeSchedule{ .optionFeePerContract = Euro(0.75),
            .shareCommissionPerTrade = Euro(2.00),
            .handlingFeePerTrade = Euro(1.00),
            .connectivityFeeCapPerExchangePerYear = Euro(2.50),
            .connectivityBasisPoints = 25,
            .currencyHandlingBasisPoints = 25 });
    }

    Box3TaxModel Rules()
    {
        return Box3TaxModel(Box3Parameters{ .taxYear = 2025,
            .taxFreeAllowance = Euro(57684.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 });
    }

    UnderlyingTerms Market(std::string underlying, double initialPrice, double volatility)
    {
        return { .underlying = std::move(underlying),
            .initialPrice = Euro(initialPrice),
            .drift = Rate::FromContinuous(0.07),
            .volatility = Volatility::FromAnnualized(volatility),
            .impliedVolatility = Volatility::FromAnnualized(volatility),
            .dividendYield = Rate::Zero(),
            .dividendWithholdingBasisPoints = 1500,
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

    Book Of(const BookScript& script, const Money& funded, std::int32_t bufferBasisPoints = 500)
    {
        return Book(script.Underlyings(), Portfolio::Funded(funded), ReinvestmentPolicy::WithCashBuffer(bufferBasisPoints), RollRules(RollTriggers{ .daysToExpiry = 7, .profitCaptureBasisPoints = 8000, .deltaLimit = Delta::FromValue(0.60) }), Fees(), Rules());
    }

    std::vector<GbmPathGenerator> Markets(const BookTerms& terms)
    {
        std::vector<GbmPathGenerator> generators;

        for (const auto& underlying : terms.underlyings)
            generators.push_back(GbmPathGenerator(GbmParameters{ .initialPrice = underlying.initialPrice,
                .drift = underlying.drift,
                .volatility = underlying.volatility,
                .convention = DayCountConvention::Act365Fixed }));

        return generators;
    }

    class CountingObserver
        : public RunObserver
    {
    public:
        explicit CountingObserver(std::int32_t stopAfter)
            : stopAfter(stopAfter)
        {}

        void Completed(std::int32_t path, std::int32_t of) override
        {
            completed.push_back(path);
            total = of;
        }

        bool Continues() const override
        {
            return static_cast<std::int32_t>(completed.size()) < stopAfter;
        }

        std::vector<std::int32_t> completed;
        std::int32_t total{ 0 };

    private:
        std::int32_t stopAfter;
    };

    class TestBookMonteCarlo
        : public ::testing::Test
    {
    protected:
        BookTerms terms{ Terms({ Market("ACME", 40.00, 0.25), Market("BETA", 55.00, 0.30) }) };
        BookScript script{ terms };
        BookMonteCarlo simulation{ Of(script, Euro(capital)), script, Markets(terms) };
    };
}

TEST_F(TestBookMonteCarlo, the_same_seed_reproduces_a_run_to_the_cent)
{
    Rng first(seed);
    Rng second(seed);

    EXPECT_EQ(simulation.Over(16, first).Equity(50), simulation.Over(16, second).Equity(50));
}

TEST_F(TestBookMonteCarlo, another_seed_produces_another_run)
{
    Rng first(seed);
    Rng second(seed + 1u);

    EXPECT_NE(simulation.Over(16, first).Equity(50), simulation.Over(16, second).Equity(50));
}

TEST_F(TestBookMonteCarlo, the_outcomes_of_a_book_are_ordered_and_reported_for_every_path)
{
    Rng rng(seed);
    const auto distribution = simulation.Over(32, rng);

    EXPECT_EQ(distribution.Paths(), 32u);
    EXPECT_LE(distribution.Equity(5), distribution.Equity(50));
    EXPECT_LE(distribution.Equity(50), distribution.Equity(95));
    EXPECT_GE(distribution.WorstDrawdown(), 0.0);
}

TEST_F(TestBookMonteCarlo, a_book_of_one_underlying_is_the_wheel_that_underlying_would_have_run)
{
    BookTerms alone{ Terms({ Market("ACME", 40.00, 0.25) }) };
    BookScript one{ alone };
    BookMonteCarlo single(Of(one, Euro(capital)), one, Markets(alone));

    Rng rng(seed);
    const auto distribution = single.Over(8, rng);

    EXPECT_EQ(distribution.Paths(), 8u);
    EXPECT_GT(distribution.MeanEquity().Cents(), 0);
}

TEST_F(TestBookMonteCarlo, a_second_uncorrelated_underlying_of_the_same_market_narrows_the_spread)
{
    BookTerms alone{ Terms({ Market("ACME", 40.00, 0.25) }) };
    BookTerms paired{ Terms({ Market("ACME", 40.00, 0.25), Market("BETA", 40.00, 0.25) }) };

    BookScript one{ alone };
    BookScript two{ paired };

    BookMonteCarlo single(Of(one, Euro(capital)), one, Markets(alone));
    BookMonteCarlo diversified(Of(two, Euro(capital)), two, Markets(paired));

    Rng first(seed);
    Rng second(seed);

    const auto narrow = diversified.Over(64, second);
    const auto wide = single.Over(64, first);

    EXPECT_LT(narrow.Equity(95) - narrow.Equity(5), wide.Equity(95) - wide.Equity(5));
}

TEST_F(TestBookMonteCarlo, a_run_reports_every_path_it_finished_in_order)
{
    CountingObserver observer(32);
    Rng rng(seed);

    const auto distribution = simulation.Over(8, rng, observer);

    EXPECT_EQ(distribution.Paths(), 8u);
    EXPECT_EQ(observer.completed, (std::vector<std::int32_t>{ 1, 2, 3, 4, 5, 6, 7, 8 }));
    EXPECT_EQ(observer.total, 8);
}

TEST_F(TestBookMonteCarlo, a_run_that_is_asked_to_stop_reports_the_paths_it_had_finished)
{
    CountingObserver observer(3);
    Rng rng(seed);

    const auto distribution = simulation.Over(64, rng, observer);

    EXPECT_EQ(distribution.Paths(), 3u);
    EXPECT_EQ(observer.completed.size(), 3u);
}

TEST_F(TestBookMonteCarlo, watching_a_run_does_not_change_it)
{
    CountingObserver observer(16);
    Rng watched(seed);
    Rng unwatched(seed);

    EXPECT_EQ(simulation.Over(16, watched, observer).Equity(50), simulation.Over(16, unwatched).Equity(50));
}

TEST_F(TestBookMonteCarlo, a_book_that_cannot_pay_a_charge_borrows_and_the_run_reports_how_often_rather_than_stopping)
{
    BookMonteCarlo committed(Of(script, Euro(capital), 0), script, Markets(terms));
    Rng rng(seed);

    const auto distribution = committed.Over(256, rng);

    EXPECT_EQ(distribution.Paths(), 256u);
    EXPECT_GE(distribution.DebtFrequency(), 0.0);
    EXPECT_LE(distribution.DebtFrequency(), 1.0);
    EXPECT_GT(distribution.At(50).positions, 0);
}

TEST_F(TestBookMonteCarlo, a_buffered_book_still_completes_the_run_that_used_to_stop_at_its_two_hundred_and_thirty_sixth_path)
{
    Rng rng(seed);

    const auto distribution = simulation.Over(256, rng);

    EXPECT_EQ(distribution.Paths(), 256u);
}
