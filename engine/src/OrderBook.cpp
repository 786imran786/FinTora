#include "OrderBook.h"

#include <cstdint>
#include <utility>
#include <vector>

// =========================================================
// ADD ORDER
// =========================================================

void OrderBook::addOrder(const Order& order)
{
    if (order.side == Side::BUY)
    {
        bids[order.price].push_back(order);
        auto it = bids[order.price].end();
        --it;
        orderLocations[order.orderId] = {Side::BUY, order.price, it};
    }
    else
    {
        asks[order.price].push_back(order);
        auto it = asks[order.price].end();
        --it;
        orderLocations[order.orderId] = {Side::SELL, order.price, it};
    }
}

// =========================================================
// BEST BID
// =========================================================

Order* OrderBook::bestBid()
{
    // If there are no BUY orders,
    // there is no best bid.
    if (bids.empty())
    {
        return nullptr;
    }

    // bids.begin() points to the highest BUY price
    // because bids uses std::greater<double>.
    auto& bestPriceLevel = bids.begin()->second;

    // Safety check.
    if (bestPriceLevel.empty())
    {
        return nullptr;
    }

    // front() gives us the oldest order
    // at the best price.
    //
    // This gives us Price-Time Priority:
    // 1. Highest price
    // 2. Earliest order at that price
    return &bestPriceLevel.front();
}

// =========================================================
// BEST BID - CONST VERSION
// =========================================================

const Order* OrderBook::bestBid() const
{
    // If there are no BUY orders,
    // there is no best bid.
    if (bids.empty())
    {
        return nullptr;
    }

    // Get the highest BUY price level.
    const auto& bestPriceLevel = bids.begin()->second;

    // Safety check.
    if (bestPriceLevel.empty())
    {
        return nullptr;
    }

    // Return the oldest order at the highest price.
    return &bestPriceLevel.front();
}

// =========================================================
// BEST ASK
// =========================================================

Order* OrderBook::bestAsk()
{
    // If there are no SELL orders,
    // there is no best ask.
    if (asks.empty())
    {
        return nullptr;
    }

    // asks.begin() points to the lowest SELL price
    // because asks uses the default ascending order.
    auto& bestPriceLevel = asks.begin()->second;

    // Safety check.
    if (bestPriceLevel.empty())
    {
        return nullptr;
    }

    // Return the oldest order at the lowest price.
    return &bestPriceLevel.front();
}

// =========================================================
// BEST ASK - CONST VERSION
// =========================================================

const Order* OrderBook::bestAsk() const
{
    // If there are no SELL orders,
    // there is no best ask.
    if (asks.empty())
    {
        return nullptr;
    }

    // Get the lowest SELL price level.
    const auto& bestPriceLevel = asks.begin()->second;

    // Safety check.
    if (bestPriceLevel.empty())
    {
        return nullptr;
    }

    // Return the oldest order at the lowest price.
    return &bestPriceLevel.front();
}

// =========================================================
// REMOVE BEST BID
// =========================================================

bool OrderBook::removeBestBid()
{
    if (bids.empty())
    {
        return false;
    }

    auto& bestPriceLevel = bids.begin()->second;

    if (bestPriceLevel.empty())
    {
        return false;
    }

    orderLocations.erase(bestPriceLevel.front().orderId);
    bestPriceLevel.pop_front();

    if (bestPriceLevel.empty())
    {
        bids.erase(bids.begin());
    }

    return true;
}

// =========================================================
// REMOVE BEST ASK
// =========================================================

bool OrderBook::removeBestAsk()
{
    if (asks.empty())
    {
        return false;
    }

    auto& bestPriceLevel = asks.begin()->second;

    if (bestPriceLevel.empty())
    {
        return false;
    }

    orderLocations.erase(bestPriceLevel.front().orderId);
    bestPriceLevel.pop_front();

    if (bestPriceLevel.empty())
    {
        asks.erase(asks.begin());
    }

    return true;
}

// =========================================================
// BID DEPTH
// =========================================================

std::vector<std::pair<double, std::uint64_t>>
OrderBook::getBidDepth(std::size_t levels) const
{
    // Stores:
    //
    // {price, total remaining quantity}
    //
    // for each BUY price level.
    std::vector<std::pair<double, std::uint64_t>> depth;

    // Number of price levels processed so far.
    std::size_t count = 0;

    // bids are already sorted:
    //
    // highest price → lowest price
    //
    // So this loop automatically gives us
    // the correct L2 order.
    for (const auto& [price, orders] : bids)
    {
        // Stop when we have collected
        // the requested number of levels.
        if (count >= levels)
        {
            break;
        }

        // Total remaining quantity at this price.
        std::uint64_t totalQuantity = 0;

        // Visit every order at this price.
        for (const auto& order : orders)
        {
            // Only remaining quantity matters.
            totalQuantity += order.remainingQuantity;
        }

        // Store:
        //
        // {price, aggregated quantity}
        depth.push_back({price, totalQuantity});

        ++count;
    }

    return depth;
}

// =========================================================
// ASK DEPTH
// =========================================================

std::vector<std::pair<double, std::uint64_t>>
OrderBook::getAskDepth(std::size_t levels) const
{
    // Stores:
    //
    // {price, total remaining quantity}
    //
    // for each SELL price level.
    std::vector<std::pair<double, std::uint64_t>> depth;

    // Number of price levels processed.
    std::size_t count = 0;

    // asks are already sorted:
    //
    // lowest price → highest price
    //
    // So this loop automatically gives us
    // the correct L2 order.
    for (const auto& [price, orders] : asks)
    {
        // Stop after requested number of levels.
        if (count >= levels)
        {
            break;
        }

        // Total remaining quantity at this price.
        std::uint64_t totalQuantity = 0;

        // Visit every order at this price.
        for (const auto& order : orders)
        {
            // Add only the remaining quantity.
            totalQuantity += order.remainingQuantity;
        }

        // Store:
        //
        // {price, aggregated quantity}
        depth.push_back({price, totalQuantity});

        ++count;
    }

    return depth;
}

// =========================================================
// CANCEL ORDER
// =========================================================

bool OrderBook::cancelOrder(std::uint64_t orderId)
{
    auto it = orderLocations.find(orderId);
    if (it == orderLocations.end())
    {
        return false;
    }

    const OrderLocation& loc = it->second;

    if (loc.side == Side::BUY)
    {
        auto& priceLevel = bids[loc.price];
        priceLevel.erase(loc.iterator);
        if (priceLevel.empty())
        {
            bids.erase(loc.price);
        }
    }
    else
    {
        auto& priceLevel = asks[loc.price];
        priceLevel.erase(loc.iterator);
        if (priceLevel.empty())
        {
            asks.erase(loc.price);
        }
    }

    orderLocations.erase(it);
    return true;
}

// =========================================================
// GET OPEN ORDERS
// =========================================================

std::vector<Order> OrderBook::getOpenOrders() const
{
    std::vector<Order> openOrders;
    for (const auto& [price, orders] : bids)
    {
        for (const auto& order : orders)
        {
            openOrders.push_back(order);
        }
    }
    for (const auto& [price, orders] : asks)
    {
        for (const auto& order : orders)
        {
            openOrders.push_back(order);
        }
    }
    return openOrders;
}