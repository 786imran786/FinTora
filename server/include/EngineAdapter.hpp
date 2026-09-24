#pragma once

#include "Protocol.hpp"
#include <vector>
#include <map>
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
// EngineAdapter: Thin adapter around MEMBER 1's MatchingEngine.
//
// MEMBER 1 INTEGRATION:
//   Replace the stub implementation in EngineAdapter.cpp with calls to the
//   real MatchingEngine. The adapter methods map directly:
//
//     placeOrder()   -> MatchingEngine::placeOrder(...)
//     cancelOrder()  -> MatchingEngine::cancelOrder(...)
//     getOrderBook() -> MatchingEngine::getOrderBook() / OrderBook snapshot
//     getActiveOrderCount() -> MatchingEngine active order count
//
//   The current stub provides minimal in-memory behavior so the server
//   compiles and basic request/response flow can be tested end-to-end.
//   It does NOT implement real price-time priority matching.
// =============================================================================
class EngineAdapter {
public:
    EngineAdapter();

    EngineResult placeOrder(Side side, OrderType orderType, double price, int64_t quantity);
    CancelResult cancelOrder(int64_t orderId);
    OrderBookSnapshot getOrderBook();
    int64_t getActiveOrderCount();

private:
    // --- STUB STATE (remove when connecting real engine) ---
    struct StubOrder {
        int64_t id;
        Side side;
        OrderType orderType;
        double price;
        int64_t quantity;
        int64_t remainingQuantity;
    };

    std::mutex mutex_;
    int64_t nextOrderId_ = 1;
    int64_t nextTradeId_ = 1;
    std::map<int64_t, StubOrder> orders_;

    EngineResult tryMatch(StubOrder& incoming);
    // --- END STUB STATE ---
};

} // namespace fintora
