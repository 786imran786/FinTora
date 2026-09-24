// =============================================================================
// EngineAdapter — wired to the real MatchingEngine
// =============================================================================

#include "EngineAdapter.hpp"

// Real matching engine headers (from ../engine/include/)
#include "MatchingEngine.h"
#include "Order.h"
#include "Event.h"

#include <algorithm>
#include <chrono>
#include <iostream>

namespace fintora {

// -------------------------------------------------------------------------
// Helpers: convert between server-protocol types and engine types
// -------------------------------------------------------------------------

static ::Side toEngineSide(Side s) {
    return (s == Side::BUY) ? ::Side::BUY : ::Side::SELL;
}

static ::OrderType toEngineOrderType(OrderType t) {
    return (t == OrderType::LIMIT) ? ::OrderType::LIMIT : ::OrderType::MARKET;
}

// Returns microseconds since epoch as a uint64_t (used for order timestamps).
static std::uint64_t nowUs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
}

// -------------------------------------------------------------------------
// EngineAdapter implementation
// -------------------------------------------------------------------------

EngineAdapter::EngineAdapter() {}

EngineResult EngineAdapter::placeOrder(Side side, OrderType orderType, double price, int64_t quantity) {
    std::lock_guard<std::mutex> lock(mutex_);

    EngineResult result;

    // Build the engine Order
    ::Order order;
    order.orderId           = nextOrderId_++;
    order.symbol            = "BTC/USDT";
    order.side              = toEngineSide(side);
    order.orderType         = toEngineOrderType(orderType);
    order.price             = price;
    order.quantity          = static_cast<std::uint64_t>(quantity);
    order.remainingQuantity = static_cast<std::uint64_t>(quantity);
    order.timestamp         = nowUs();

    // Submit to the real matching engine
    std::vector<::Event> events = engine_.placeOrder(order);

    result.orderId   = static_cast<int64_t>(order.orderId);
    result.accepted  = true;

    int64_t remaining = quantity; // will be updated from events

    for (const auto& ev : events) {
        switch (ev.type) {
            case EventType::ORDER_ACCEPTED:
                // Nothing extra needed
                break;

            case EventType::TRADE_EXECUTED: {
                TradeEvent te;
                te.tradeId  = static_cast<int64_t>(ev.trade.tradeId);
                te.price    = ev.trade.price;
                te.quantity = static_cast<int64_t>(ev.trade.quantity);
                result.trades.push_back(te);

                // Update remaining for the incoming order
                if (ev.trade.buyOrderId == order.orderId ||
                    ev.trade.sellOrderId == order.orderId) {
                    remaining -= static_cast<int64_t>(ev.trade.quantity);
                }
                break;
            }

            case EventType::ORDER_COMPLETELY_FILLED: {
                // If this is the incoming order being filled
                if (ev.orderId == order.orderId) {
                    result.status            = OrderStatus::FILLED;
                    result.remainingQuantity = 0;
                    result.orderUpdates.push_back({result.orderId, OrderStatus::FILLED, 0});
                } else {
                    // A resting order was completely filled
                    result.orderUpdates.push_back(
                        {static_cast<int64_t>(ev.orderId), OrderStatus::FILLED, 0});
                }
                break;
            }

            case EventType::ORDER_PARTIALLY_FILLED: {
                if (ev.orderId == order.orderId) {
                    result.status            = OrderStatus::PARTIALLY_FILLED;
                    result.remainingQuantity = remaining;
                    result.orderUpdates.push_back(
                        {result.orderId, OrderStatus::PARTIALLY_FILLED, remaining});
                } else {
                    // We don't know exact remaining for the resting order from the event,
                    // but we can query the open orders list. For now emit a partial update
                    // with quantity=0 as a sentinel; the ORDER_BOOK broadcast covers it.
                    result.orderUpdates.push_back(
                        {static_cast<int64_t>(ev.orderId), OrderStatus::PARTIALLY_FILLED, -1});
                }
                break;
            }

            case EventType::ORDER_BOOK_CHANGED:
                result.bookChanged = true;
                break;

            default:
                break;
        }
    }

    // If no fill/partial events fired for the incoming order, it was accepted and rests
    if (result.status == OrderStatus::ACCEPTED) {
        result.remainingQuantity = remaining;
        // Only LIMIT orders rest; MARKET orders that didn't fill get rejected for no liquidity
        if (orderType == OrderType::MARKET && remaining == quantity) {
            result.accepted     = false;
            result.rejectReason = "No liquidity for market order";
        }
    }

    return result;
}

CancelResult EngineAdapter::cancelOrder(int64_t orderId) {
    std::lock_guard<std::mutex> lock(mutex_);

    CancelResult result;
    result.orderId = orderId;

    std::vector<::Event> events = engine_.cancelOrder(static_cast<std::uint64_t>(orderId));

    if (events.empty()) {
        result.success = false;
        result.reason  = "Order not found";
        return result;
    }

    result.success = true;
    return result;
}

OrderBookSnapshot EngineAdapter::getOrderBook() {
    std::lock_guard<std::mutex> lock(mutex_);

    constexpr std::size_t DEPTH_LEVELS = 10;
    const auto& ob = engine_.getOrderBook();

    OrderBookSnapshot snapshot;

    for (const auto& [price, qty] : ob.getBidDepth(DEPTH_LEVELS)) {
        snapshot.bids.push_back({price, static_cast<int64_t>(qty)});
    }
    for (const auto& [price, qty] : ob.getAskDepth(DEPTH_LEVELS)) {
        snapshot.asks.push_back({price, static_cast<int64_t>(qty)});
    }

    return snapshot;
}

int64_t EngineAdapter::getActiveOrderCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int64_t>(engine_.getOpenOrders().size());
}

} // namespace fintora
