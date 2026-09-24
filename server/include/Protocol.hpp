#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <stdexcept>

using json = nlohmann::json;

namespace fintora {

enum class Side { BUY, SELL };
enum class OrderType { LIMIT, MARKET };
enum class OrderStatus { ACCEPTED, PARTIALLY_FILLED, FILLED, CANCELLED };

struct PlaceOrderRequest {
    Side side;
    OrderType orderType;
    double price = 0.0;
    int64_t quantity = 0;
};

struct CancelOrderRequest {
    int64_t orderId;
};

struct OrderAcceptedResponse {
    int64_t orderId;
};

struct OrderRejectedResponse {
    std::string reason;
};

struct OrderCancelledResponse {
    int64_t orderId;
};

struct TradeEvent {
    int64_t tradeId;
    double price;
    int64_t quantity;
};

struct OrderUpdateEvent {
    int64_t orderId;
    OrderStatus status;
    int64_t remainingQuantity;
};

struct PriceLevel {
    double price;
    int64_t quantity;
};

struct OrderBookSnapshot {
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
};

struct MetricsData {
    int64_t ordersProcessed = 0;
    int64_t tradesExecuted = 0;
    int64_t activeOrders = 0;
    int64_t totalVolume = 0;
    double ordersPerSecond = 0.0;
};

struct ErrorResponse {
    std::string message;
};

class ProtocolError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Protocol {
public:
    static json parse(const std::string& raw);

    static std::string getMessageType(const json& msg);
    static PlaceOrderRequest parsePlaceOrder(const json& msg);
    static CancelOrderRequest parseCancelOrder(const json& msg);

    static json orderAccepted(int64_t orderId);
    static json orderRejected(const std::string& reason);
    static json orderCancelled(int64_t orderId);
    static json tradeEvent(const TradeEvent& trade);
    static json orderUpdate(const OrderUpdateEvent& update);
    static json orderBook(const OrderBookSnapshot& snapshot);
    static json metrics(const MetricsData& data);
    static json error(const std::string& message);

    static std::string sideToString(Side side);
    static std::string orderStatusToString(OrderStatus status);

private:
    static Side parseSide(const std::string& s);
    static OrderType parseOrderType(const std::string& s);
    static void validatePlaceOrder(const json& msg);
    static void validateCancelOrder(const json& msg);
};

} // namespace fintora
