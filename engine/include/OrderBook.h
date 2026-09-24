#pragma once

#include "Order.h"

#include <cstddef>
#include <cstdint>
#include <list>
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>

class OrderBook {
public:

    // Adds an order to the correct side of the order book.
    void addOrder(const Order& order);

    // Returns the highest-priority BUY order.
    // Non-const version allows the MatchingEngine to modify it.
    Order* bestBid();

    // Const version allows read-only access.
    const Order* bestBid() const;

    // Returns the highest-priority SELL order.
    // Non-const version allows the MatchingEngine to modify it.
    Order* bestAsk();

    // Const version allows read-only access.
    const Order* bestAsk() const;

    // Removes the highest-priority BUY order.
    bool removeBestBid();

    // Removes the highest-priority SELL order.
    bool removeBestAsk();

    // Cancels an order given its ID
    bool cancelOrder(std::uint64_t orderId);

    // Returns aggregated BUY-side L2 depth.
    std::vector<std::pair<double, std::uint64_t>> getBidDepth(
        std::size_t levels
    ) const;

    // Returns aggregated SELL-side L2 depth.
    std::vector<std::pair<double, std::uint64_t>> getAskDepth(
        std::size_t levels
    ) const;

    // Returns all resting orders
    std::vector<Order> getOpenOrders() const;

private:

    // BUY prices are sorted from highest to lowest.
    std::map<double, std::list<Order>, std::greater<double>> bids;

    // SELL prices are sorted from lowest to highest.
    std::map<double, std::list<Order>> asks;

    struct OrderLocation {
        Side side;
        double price;
        std::list<Order>::iterator iterator;
    };

    // Fast lookup for order cancellation
    std::unordered_map<std::uint64_t, OrderLocation> orderLocations;
};