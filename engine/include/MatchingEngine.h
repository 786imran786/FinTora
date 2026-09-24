#pragma once

#include "Order.h"
#include "Trade.h"
#include "Event.h"
#include "OrderBook.h"

#include <vector>

class MatchingEngine
{
public:

    // Submit an order to the matching engine.
    std::vector<Event> placeOrder(Order order);

    // Cancel an order by ID.
    std::vector<Event> cancelOrder(std::uint64_t orderId);

    // Allows other parts of the system to inspect the order book.
    const OrderBook& getOrderBook() const;

    // Get open resting orders.
    std::vector<Order> getOpenOrders() const;

    // Get recent trades.
    std::vector<Trade> getRecentTrades() const;

private:

    // The order book stores currently resting orders.
    OrderBook orderBook;

    // Recent trades
    std::vector<Trade> recentTrades;

    // Generates unique trade IDs.
    std::uint64_t nextTradeId = 1;

    // Generates the next timestamp.
    std::uint64_t nextTimestamp = 1;
};