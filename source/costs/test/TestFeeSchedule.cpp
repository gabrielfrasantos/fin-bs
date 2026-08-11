#include "source/costs/FeeSchedule.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::core::Currency;
    using fin::core::Money;
    using fin::costs::CurrencyOf;
    using fin::costs::FeeSchedule;
    using fin::costs::IsWellFormed;

    FeeSchedule Published()
    {
        return FeeSchedule{
            .optionFeePerContract = Money::FromCents(75, Currency::Eur),
            .shareCommissionPerTrade = Money::FromCents(200, Currency::Eur),
            .handlingFeePerTrade = Money::FromCents(100, Currency::Eur),
            .connectivityFeeCapPerExchangePerYear = Money::FromCents(250, Currency::Eur),
            .connectivityBasisPoints = 25,
            .currencyHandlingBasisPoints = 25
        };
    }
}

TEST(TestFeeSchedule, the_published_schedule_is_well_formed)
{
    EXPECT_TRUE(IsWellFormed(Published()));
    EXPECT_EQ(CurrencyOf(Published()), Currency::Eur);
}

TEST(TestFeeSchedule, a_schedule_quoted_in_two_currencies_is_not_well_formed)
{
    auto schedule = Published();
    schedule.handlingFeePerTrade = Money::FromCents(100, Currency::Usd);

    EXPECT_FALSE(IsWellFormed(schedule));
}

TEST(TestFeeSchedule, a_negative_fee_is_not_well_formed)
{
    auto schedule = Published();
    schedule.optionFeePerContract = Money::FromCents(-1, Currency::Eur);

    EXPECT_FALSE(IsWellFormed(schedule));
}

TEST(TestFeeSchedule, a_free_schedule_is_well_formed)
{
    const FeeSchedule free{
        .optionFeePerContract = Money::Zero(Currency::Eur),
        .shareCommissionPerTrade = Money::Zero(Currency::Eur),
        .handlingFeePerTrade = Money::Zero(Currency::Eur),
        .connectivityFeeCapPerExchangePerYear = Money::Zero(Currency::Eur),
        .connectivityBasisPoints = 0,
        .currencyHandlingBasisPoints = 0
    };

    EXPECT_TRUE(IsWellFormed(free));
}

TEST(TestFeeSchedule, a_proportional_component_beyond_the_whole_amount_is_not_well_formed)
{
    auto beyondEverything = Published();
    beyondEverything.connectivityBasisPoints = 10001;

    auto negative = Published();
    negative.currencyHandlingBasisPoints = -1;

    EXPECT_FALSE(IsWellFormed(beyondEverything));
    EXPECT_FALSE(IsWellFormed(negative));
}

TEST(TestFeeSchedule, the_currency_of_a_malformed_schedule_is_refused)
{
    auto schedule = Published();
    schedule.shareCommissionPerTrade = Money::FromCents(200, Currency::Usd);

    EXPECT_DEATH(CurrencyOf(schedule), "");
}
