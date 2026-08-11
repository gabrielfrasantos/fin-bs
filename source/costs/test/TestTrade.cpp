#include "source/costs/Trade.hpp"
#include <gtest/gtest.h>

namespace
{
    using fin::costs::Instrument;
    using fin::costs::Trade;
}

TEST(TestTrade, a_share_trade_carries_its_quantity)
{
    const auto trade = Trade::Shares(100);

    EXPECT_EQ(trade.Traded(), Instrument::Share);
    EXPECT_EQ(trade.Quantity(), 100);
}

TEST(TestTrade, an_option_trade_carries_its_contract_count)
{
    const auto trade = Trade::Options(3);

    EXPECT_EQ(trade.Traded(), Instrument::Option);
    EXPECT_EQ(trade.Quantity(), 3);
}

TEST(TestTrade, trades_of_the_same_instrument_and_size_are_the_same_trade)
{
    EXPECT_EQ(Trade::Options(2), Trade::Options(2));
    EXPECT_NE(Trade::Options(2), Trade::Options(3));
    EXPECT_NE(Trade::Options(2), Trade::Shares(2));
}

TEST(TestTrade, a_trade_of_nothing_is_not_a_trade)
{
    EXPECT_DEATH(Trade::Shares(0), "");
    EXPECT_DEATH(Trade::Options(0), "");
    EXPECT_DEATH(Trade::Options(-1), "");
}
