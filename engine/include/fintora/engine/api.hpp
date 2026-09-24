#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fintora::engine {

using OrderId = std::uint64_t;
using TradeId = std::uint64_t;
using Price = std::uint64_t;
using Quantity = std::uint64_t;
using Sequence = std::uint64_t;

enum class Side {
    Buy,
    Sell,
};

enum class OrderType {
    Limit,
    Market,
};

enum class OrderStatus {
    Active,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected,
};

struct Order {
    OrderId id{};
    Side side{};
    OrderType type{};
    Quantity quantity{};
    std::optional<Price> price{};
};

struct Trade {
    TradeId id{};
    Price executionPrice{};
    Quantity executionQuantity{};
    OrderId buyOrderId{};
    OrderId sellOrderId{};
    Sequence sequence{};
};

struct OrderView {
    Order order{};
    Quantity remainingQuantity{};
    OrderStatus status{OrderStatus::Rejected};
};

struct PriceLevel {
    Price price{};
    Quantity aggregateQuantity{};
};

struct OrderBookSnapshot {
    // Bids are ordered highest price first; asks are ordered lowest price first.
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    std::optional<Price> bestBid;
    std::optional<Price> bestAsk;
};

struct SubmissionResult {
    bool accepted{false};
    OrderStatus status{OrderStatus::Rejected};
    Quantity remainingQuantity{};
    std::vector<Trade> trades;
    std::string rejectionReason;
};

struct CancellationResult {
    bool cancelled{false};
    OrderStatus status{OrderStatus::Rejected};
    Quantity remainingQuantity{};
};

class IMatchingEngine {
public:
    virtual ~IMatchingEngine() = default;

    virtual SubmissionResult submitOrder(const Order& order) = 0;
    virtual CancellationResult cancelOrder(OrderId orderId) = 0;
    virtual std::optional<OrderView> getOrder(OrderId orderId) const = 0;
    virtual OrderBookSnapshot getOrderBookSnapshot() const = 0;
};

}  // namespace fintora::engine