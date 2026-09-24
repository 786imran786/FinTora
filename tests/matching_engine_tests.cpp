#include <gtest/gtest.h>

#include <type_traits>

#include "fintora/engine/api.hpp"

namespace fintora::engine {
namespace {

TEST(PublicApiContract, UsesIntegerExactMatchingValues) {
    static_assert(std::is_integral_v<Price>);
    static_assert(std::is_integral_v<Quantity>);
    static_assert(std::is_same_v<OrderId, std::uint64_t>);

    SUCCEED();
}

TEST(PublicApiContract, RepresentsLimitAndMarketOrders) {
    const Order limit{42, Side::Buy, OrderType::Limit, 100, Price{101}};
    const Order market{43, Side::Sell, OrderType::Market, 75, std::nullopt};

    EXPECT_EQ(limit.id, 42);
    EXPECT_EQ(limit.price, Price{101});
    EXPECT_EQ(market.type, OrderType::Market);
    EXPECT_FALSE(market.price.has_value());
}

TEST(PublicApiContract, RepresentsTradesAndBookLevels) {
    const Trade trade{7, 101, 50, 42, 43, 1};
    const OrderBookSnapshot snapshot{
        {{100, 25}},
        {{101, 50}},
        Price{100},
        Price{101},
    };

    EXPECT_EQ(trade.executionPrice, 101);
    EXPECT_EQ(trade.executionQuantity, 50);
    ASSERT_EQ(snapshot.bids.size(), 1);
    ASSERT_EQ(snapshot.asks.size(), 1);
    EXPECT_EQ(snapshot.bids.front().aggregateQuantity, 25);
    EXPECT_EQ(snapshot.asks.front().aggregateQuantity, 50);
}

TEST(PublicApiContract, EngineInterfaceIsAbstract) {
    static_assert(std::is_abstract_v<IMatchingEngine>);
    SUCCEED();
}

}  // namespace
}  // namespace fintora::engine