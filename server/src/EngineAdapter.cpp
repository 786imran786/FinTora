// =============================================================================
// STUB EngineAdapter
//
// This is a TEMPORARY stub so the server compiles without MEMBER 1's engine.
// It provides minimal order storage and basic matching to enable end-to-end
// testing of the WebSocket/JSON layer.
//
// MEMBER 1 INTEGRATION:
//   1. Include MEMBER 1's MatchingEngine headers.
//   2. Replace the stub internals with calls to the real engine.
//   3. Remove StubOrder, the orders_ map, and tryMatch().
//   4. The public interface (placeOrder, cancelOrder, getOrderBook,
//      getActiveOrderCount) stays the same — only the implementation changes.
// =============================================================================

#include "EngineAdapter.hpp"
#include <algorithm>
#include <iostream>

namespace fintora {

EngineAdapter::EngineAdapter() {}

EngineResult EngineAdapter::placeOrder(Side side, OrderType orderType, double price, int64_t quantity) {
    std::lock_guard<std::mutex> lock(mutex_);

    EngineResult result;

    StubOrder order;
    order.id = nextOrderId_++;
    order.side = side;
    order.orderType = orderType;
    order.price = price;
    order.quantity = quantity;
    order.remainingQuantity = quantity;

    result.orderId = order.id;
    result.accepted = true;

    EngineResult matchResult = tryMatch(order);
    result.trades = std::move(matchResult.trades);
    result.orderUpdates = std::move(matchResult.orderUpdates);

    if (order.remainingQuantity > 0 && orderType == OrderType::LIMIT) {
        orders_[order.id] = order;
        result.status = (order.remainingQuantity < quantity)
            ? OrderStatus::PARTIALLY_FILLED
            : OrderStatus::ACCEPTED;
        result.remainingQuantity = order.remainingQuantity;
        result.bookChanged = true;

        if (result.status == OrderStatus::PARTIALLY_FILLED) {
            result.orderUpdates.push_back({order.id, OrderStatus::PARTIALLY_FILLED, order.remainingQuantity});
        }
    } else if (order.remainingQuantity == 0) {
        result.status = OrderStatus::FILLED;
        result.remainingQuantity = 0;
        result.orderUpdates.push_back({order.id, OrderStatus::FILLED, 0});
        result.bookChanged = true;
    } else {
        if (orderType == OrderType::MARKET && order.remainingQuantity > 0) {
            if (order.remainingQuantity == quantity) {
                result.accepted = false;
                result.rejectReason = "No liquidity for market order";
                return result;
            }
            result.status = OrderStatus::FILLED;
            result.remainingQuantity = 0;
            result.orderUpdates.push_back({order.id, OrderStatus::FILLED, 0});
            result.bookChanged = !result.trades.empty();
        }
    }

    if (!result.trades.empty()) {
        result.bookChanged = true;
    }

    return result;
}

EngineResult EngineAdapter::tryMatch(StubOrder& incoming) {
    EngineResult result;

    auto it = orders_.begin();
    while (it != orders_.end() && incoming.remainingQuantity > 0) {
        auto& resting = it->second;

        bool canMatch = false;
        if (incoming.side == Side::BUY && resting.side == Side::SELL) {
            if (incoming.orderType == OrderType::MARKET || incoming.price >= resting.price) {
                canMatch = true;
            }
        } else if (incoming.side == Side::SELL && resting.side == Side::BUY) {
            if (incoming.orderType == OrderType::MARKET || incoming.price <= resting.price) {
                canMatch = true;
            }
        }

        if (canMatch) {
            int64_t fillQty = std::min(incoming.remainingQuantity, resting.remainingQuantity);
            double fillPrice = resting.price;

            TradeEvent trade;
            trade.tradeId = nextTradeId_++;
            trade.price = fillPrice;
            trade.quantity = fillQty;
            result.trades.push_back(trade);

            incoming.remainingQuantity -= fillQty;
            resting.remainingQuantity -= fillQty;

            if (resting.remainingQuantity == 0) {
                result.orderUpdates.push_back({resting.id, OrderStatus::FILLED, 0});
                it = orders_.erase(it);
            } else {
                result.orderUpdates.push_back({resting.id, OrderStatus::PARTIALLY_FILLED, resting.remainingQuantity});
                ++it;
            }
        } else {
            ++it;
        }
    }

    return result;
}

CancelResult EngineAdapter::cancelOrder(int64_t orderId) {
    std::lock_guard<std::mutex> lock(mutex_);

    CancelResult result;
    result.orderId = orderId;

    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        result.success = false;
        result.reason = "Order not found";
        return result;
    }

    orders_.erase(it);
    result.success = true;
    return result;
}

OrderBookSnapshot EngineAdapter::getOrderBook() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<double, int64_t, std::greater<>> bidLevels;
    std::map<double, int64_t> askLevels;

    for (const auto& [id, order] : orders_) {
        if (order.side == Side::BUY) {
            bidLevels[order.price] += order.remainingQuantity;
        } else {
            askLevels[order.price] += order.remainingQuantity;
        }
    }

    OrderBookSnapshot snapshot;
    for (const auto& [price, qty] : bidLevels) {
        snapshot.bids.push_back({price, qty});
    }
    for (const auto& [price, qty] : askLevels) {
        snapshot.asks.push_back({price, qty});
    }

    return snapshot;
}

int64_t EngineAdapter::getActiveOrderCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int64_t>(orders_.size());
}

} // namespace fintora
