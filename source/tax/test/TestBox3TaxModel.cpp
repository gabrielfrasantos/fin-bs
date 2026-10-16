#include "source/tax/Box3TaxModel.hpp"
#include <cstdint>
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::tax::Box3Assets;
    using fin::tax::Box3Parameters;
    using fin::tax::Box3TaxModel;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    Box3Parameters PublishedForTwentyTwentyFive()
    {
        return { .taxYear = 2025,
            .taxFreeAllowance = Euro(57684.00),
            .depositReturnBasisPoints = 137,
            .investmentReturnBasisPoints = 588,
            .taxBasisPoints = 3600,
            .dividendWithholdingBasisPoints = 1500 };
    }

    Box3Assets Held(double deposits, double investments)
    {
        return { Euro(deposits), Euro(investments) };
    }

    Box3TaxModel Malformed(std::int32_t taxBasisPoints)
    {
        auto parameters = PublishedForTwentyTwentyFive();
        parameters.taxBasisPoints = taxBasisPoints;

        return Box3TaxModel(parameters);
    }

    class TestBox3TaxModel
        : public ::testing::Test
    {
    protected:
        Box3TaxModel model{ PublishedForTwentyTwentyFive() };
    };
}

TEST_F(TestBox3TaxModel, the_model_states_the_year_whose_rules_it_applies)
{
    EXPECT_EQ(model.Year(), 2025);
}

TEST_F(TestBox3TaxModel, a_worked_example_of_a_hundred_thousand_split_between_a_bank_and_the_market)
{
    const auto assets = Held(20000.00, 80000.00);

    EXPECT_EQ(TotalValue(assets), Euro(100000.00));
    EXPECT_EQ(model.DeemedReturn(assets), Euro(4978.00));
    EXPECT_EQ(model.TaxableBase(assets), Euro(42316.00));
    EXPECT_EQ(model.TaxableReturn(assets), Euro(2106.49));
    EXPECT_EQ(model.Charge(assets), Euro(758.34));
}

TEST_F(TestBox3TaxModel, the_deemed_return_of_each_kind_of_asset_is_the_published_percentage_of_it)
{
    EXPECT_EQ(model.DeemedReturn(Held(20000.00, 0.00)), Euro(274.00));
    EXPECT_EQ(model.DeemedReturn(Held(0.00, 80000.00)), Euro(4704.00));
}

TEST_F(TestBox3TaxModel, the_deemed_return_of_a_mixture_is_the_sum_of_its_parts)
{
    EXPECT_EQ(model.DeemedReturn(Held(20000.00, 80000.00)), model.DeemedReturn(Held(20000.00, 0.00)) + model.DeemedReturn(Held(0.00, 80000.00)));
}

TEST_F(TestBox3TaxModel, the_same_wealth_held_as_investments_rather_than_deposits_is_taxed_more)
{
    EXPECT_EQ(model.Charge(Held(0.00, 100000.00)), Euro(895.74));
    EXPECT_EQ(model.Charge(Held(100000.00, 0.00)), Euro(208.70));
    EXPECT_GT(model.Charge(Held(0.00, 100000.00)), model.Charge(Held(50000.00, 50000.00)));
    EXPECT_GT(model.Charge(Held(50000.00, 50000.00)), model.Charge(Held(100000.00, 0.00)));
}

TEST_F(TestBox3TaxModel, wealth_exactly_at_the_allowance_is_not_taxed)
{
    EXPECT_EQ(model.TaxableBase(Held(0.00, 57684.00)), Euro(0.00));
    EXPECT_EQ(model.TaxableReturn(Held(0.00, 57684.00)), Euro(0.00));
    EXPECT_EQ(model.Charge(Held(0.00, 57684.00)), Euro(0.00));
}

TEST_F(TestBox3TaxModel, wealth_below_the_allowance_is_not_taxed_however_it_is_held)
{
    EXPECT_EQ(model.Charge(Held(57683.99, 0.00)), Euro(0.00));
    EXPECT_EQ(model.Charge(Held(0.00, 57683.99)), Euro(0.00));
    EXPECT_EQ(model.Charge(Held(30000.00, 27683.99)), Euro(0.00));
}

TEST_F(TestBox3TaxModel, wealth_of_nothing_at_all_is_not_taxed)
{
    EXPECT_EQ(model.DeemedReturn(Held(0.00, 0.00)), Euro(0.00));
    EXPECT_EQ(model.TaxableBase(Held(0.00, 0.00)), Euro(0.00));
    EXPECT_EQ(model.Charge(Held(0.00, 0.00)), Euro(0.00));
}

TEST_F(TestBox3TaxModel, the_charge_grows_from_nothing_as_wealth_passes_the_allowance)
{
    EXPECT_EQ(model.TaxableBase(Held(0.00, 57684.01)), Euro(0.01));
    EXPECT_EQ(model.Charge(Held(0.00, 57684.01)), Euro(0.00));
    EXPECT_GT(model.Charge(Held(0.00, 60000.00)), Euro(0.00));
}

TEST_F(TestBox3TaxModel, more_wealth_is_never_charged_less_tax)
{
    auto previous = Euro(0.00);

    for (auto wealth = 50000.00; wealth <= 250000.00; wealth += 2500.00)
    {
        const auto charge = model.Charge(Held(wealth / 4.0, wealth * 3.0 / 4.0));

        EXPECT_GE(charge, previous);
        previous = charge;
    }
}

TEST_F(TestBox3TaxModel, the_taxable_return_never_exceeds_the_deemed_return)
{
    const auto assets = Held(20000.00, 80000.00);

    EXPECT_LE(model.TaxableReturn(assets), model.DeemedReturn(assets));
}

TEST_F(TestBox3TaxModel, a_year_whose_rules_are_not_a_fraction_of_the_whole_amount_is_refused)
{
    EXPECT_DEATH(Malformed(10001), "");
}

TEST_F(TestBox3TaxModel, wealth_cannot_be_negative)
{
    EXPECT_DEATH(TotalValue(Held(-1.00, 0.00)), "");
    EXPECT_DEATH(TotalValue(Held(0.00, -1.00)), "");
}

TEST_F(TestBox3TaxModel, wealth_in_another_currency_than_the_rules_were_written_in_is_refused)
{
    const Box3Assets foreign{ Money::FromAmount(100000.00, Currency::Usd), Money::FromAmount(0.00, Currency::Usd) };

    EXPECT_DEATH(model.Charge(foreign), "");
}
