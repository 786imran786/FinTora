#include "Protocol.hpp"
#include <stdexcept>

namespace fintora {

json Protocol::parse(const std::string& raw) {
    if (raw.empty()) {
        throw ProtocolError("Empty message");
    }
    try {
        return json::parse(raw);
    } catch (const json::parse_error&) {
        throw ProtocolError("Invalid JSON");
    }
}

std::string Protocol::getMessageType(const json& msg) {
    if (!msg.contains("type") || !msg["type"].is_string()) {
        throw ProtocolError("Missing or invalid 'type' field");
    }
    return msg["type"].get<std::string>();
}

Side Protocol::parseSide(const std::string& s) {
    if (s == "BUY") return Side::BUY;
    if (s == "SELL") return Side::SELL;
    throw ProtocolError("Invalid side: " + s);
}

OrderType Protocol::parseOrderType(const std::string& s) {
    if (s == "LIMIT") return OrderType::LIMIT;
    if (s == "MARKET") return OrderType::MARKET;
    throw ProtocolError("Invalid orderType: " + s);
}

void Protocol::validatePlaceOrder(const json& msg) {
    if (!msg.contains("side") || !msg["side"].is_string()) {
        throw ProtocolError("Missing or invalid 'side' field");
    }
    if (!msg.contains("orderType") || !msg["orderType"].is_string()) {
        throw ProtocolError("Missing or invalid 'orderType' field");
    }
    if (!msg.contains("quantity") || !msg["quantity"].is_number()) {
        throw ProtocolError("Missing or invalid 'quantity' field");
    }

    int64_t qty = msg["quantity"].get<int64_t>();
    if (qty <= 0) {
        throw ProtocolError("Quantity must be greater than 0");
    }

    std::string orderType = msg["orderType"].get<std::string>();
    if (orderType == "LIMIT") {
        if (!msg.contains("price") || !msg["price"].is_number()) {
            throw ProtocolError("Missing or invalid 'price' field for LIMIT order");
        }
        double price = msg["price"].get<double>();
        if (price <= 0) {
            throw ProtocolError("Price must be greater than 0");
        }
    }
}

void Protocol::validateCancelOrder(const json& msg) {
    if (!msg.contains("orderId") || !msg["orderId"].is_number()) {
        throw ProtocolError("Missing or invalid 'orderId' field");
    }
}

PlaceOrderRequest Protocol::parsePlaceOrder(const json& msg) {
    validatePlaceOrder(msg);

    PlaceOrderRequest req;
    req.side = parseSide(msg["side"].get<std::string>());
    req.orderType = parseOrderType(msg["orderType"].get<std::string>());
    req.quantity = msg["quantity"].get<int64_t>();

    if (req.orderType == OrderType::LIMIT) {
        req.price = msg["price"].get<double>();
    }

    return req;
}

CancelOrderRequest Protocol::parseCancelOrder(const json& msg) {
    validateCancelOrder(msg);

    CancelOrderRequest req;
    req.orderId = msg["orderId"].get<int64_t>();
    return req;
}

json Protocol::orderAccepted(int64_t orderId) {
    return {
        {"type", "ORDER_ACCEPTED"},
        {"orderId", orderId}
    };
}

json Protocol::orderRejected(const std::string& reason) {
    return {
        {"type", "ORDER_REJECTED"},
        {"reason", reason}
    };
}

json Protocol::orderCancelled(int64_t orderId) {
    return {
        {"type", "ORDER_CANCELLED"},
        {"orderId", orderId}
    };
}

json Protocol::tradeEvent(const TradeEvent& trade) {
    return {
        {"type", "TRADE"},
        {"tradeId", trade.tradeId},
        {"price", trade.price},
        {"quantity", trade.quantity}
    };
}

json Protocol::orderUpdate(const OrderUpdateEvent& update) {
    return {
        {"type", "ORDER_UPDATE"},
        {"orderId", update.orderId},
        {"status", orderStatusToString(update.status)},
        {"remainingQuantity", update.remainingQuantity}
    };
}

json Protocol::orderBook(const OrderBookSnapshot& snapshot) {
    json bids = json::array();
    for (const auto& level : snapshot.bids) {
        bids.push_back({level.price, level.quantity});
    }

    json asks = json::array();
    for (const auto& level : snapshot.asks) {
        asks.push_back({level.price, level.quantity});
    }

    return {
        {"type", "ORDER_BOOK"},
        {"bids", bids},
        {"asks", asks}
    };
}

json Protocol::metrics(const MetricsData& data) {
    return {
        {"type", "METRICS"},
        {"ordersProcessed", data.ordersProcessed},
        {"tradesExecuted", data.tradesExecuted},
        {"activeOrders", data.activeOrders},
        {"totalVolume", data.totalVolume},
        {"ordersPerSecond", data.ordersPerSecond}
    };
}

json Protocol::error(const std::string& message) {
    return {
        {"type", "ERROR"},
        {"message", message}
    };
}

std::string Protocol::sideToString(Side side) {
    switch (side) {
        case Side::BUY: return "BUY";
        case Side::SELL: return "SELL";
    }
    return "UNKNOWN";
}

std::string Protocol::orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::ACCEPTED: return "ACCEPTED";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

} // namespace fintora
