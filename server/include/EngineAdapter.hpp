#pragma once

#include "Protocol.hpp"

// Real engine
#include "MatchingEngine.h"

#include <vector>
#include <mutex>
#include <cstdint>

namespace fintora {

struct EngineResult {
    bool accepted = false;
    std::string rejectReason;
    int64_t orderId = 0;
    int64_t remainingQuantity = 0;
    OrderStatus status = OrderStatus::ACCEPTED;
    std::vector<TradeEvent> trades;
    std::vector<OrderUpdateEvent> orderUpdates;
    bool bookChanged = false;
};

struct CancelResult {
    bool success = false;
    std::string reason;
    int64_t orderId = 0;
};

// =============================================================================
// EngineAdapter: thin adapter around the real MatchingEngine.
// =============================================================================
class EngineAdapter {
public:
    EngineAdapter();

    EngineResult placeOrder(Side side, OrderType orderType, double price, int64_t quantity);
    CancelResult cancelOrder(int64_t orderId);
    OrderBookSnapshot getOrderBook();
    int64_t getActiveOrderCount();

private:
    std::mutex mutex_;
    ::MatchingEngine engine_;      // Real price-time priority engine
    int64_t nextOrderId_ = 1;
};

} // namespace fintora
