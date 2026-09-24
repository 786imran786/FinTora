#include "RequestHandler.hpp"
#include <iostream>

namespace fintora {

RequestHandler::RequestHandler(EngineAdapter& engine, Metrics& metrics, ClientManager& clients)
    : engine_(engine), metrics_(metrics), clients_(clients) {}

HandleResult RequestHandler::handle(const std::string& rawMessage) {
    HandleResult result;

    json msg;
    try {
        msg = Protocol::parse(rawMessage);
    } catch (const ProtocolError& e) {
        result.directResponse = Protocol::error(e.what());
        return result;
    }

    std::string type;
    try {
        type = Protocol::getMessageType(msg);
    } catch (const ProtocolError& e) {
        result.directResponse = Protocol::error(e.what());
        return result;
    }

    try {
        if (type == "PLACE_ORDER") {
            return handlePlaceOrder(msg);
        } else if (type == "CANCEL_ORDER") {
            return handleCancelOrder(msg);
        } else if (type == "GET_METRICS") {
            return handleGetMetrics();
        } else {
            result.directResponse = Protocol::error("Unknown message type: " + type);
            return result;
        }
    } catch (const ProtocolError& e) {
        result.directResponse = Protocol::error(e.what());
    } catch (const std::exception& e) {
        result.directResponse = Protocol::error(std::string("Internal error: ") + e.what());
    }

    return result;
}

HandleResult RequestHandler::handlePlaceOrder(const json& msg) {
    HandleResult result;

    auto req = Protocol::parsePlaceOrder(msg);

    EngineResult engineResult;
    {
        std::lock_guard<std::mutex> lock(engineMutex_);
        engineResult = engine_.placeOrder(req.side, req.orderType, req.price, req.quantity);
        metrics_.setActiveOrders(engine_.getActiveOrderCount());
    }

    if (!engineResult.accepted) {
        result.directResponse = Protocol::orderRejected(engineResult.rejectReason);
        return result;
    }

    metrics_.recordOrder();

    // Enrich ORDER_ACCEPTED with the original request fields so the
    // frontend can immediately add the order to the Open Orders list.
    json accepted = Protocol::orderAccepted(engineResult.orderId);
    accepted["side"]      = Protocol::sideToString(req.side);
    accepted["orderType"] = (req.orderType == OrderType::LIMIT) ? "LIMIT" : "MARKET";
    accepted["quantity"]  = req.quantity;
    if (req.orderType == OrderType::LIMIT) {
        accepted["price"] = req.price;
    }
    result.directResponse = accepted;

    for (const auto& trade : engineResult.trades) {
        result.broadcasts.push_back(Protocol::tradeEvent(trade));
        metrics_.recordTrade(trade.quantity);
    }

    for (const auto& update : engineResult.orderUpdates) {
        if (update.remainingQuantity >= 0) {  // skip sentinel -1 updates
            result.broadcasts.push_back(Protocol::orderUpdate(update));
        }
    }

    if (engineResult.bookChanged) {
        result.broadcasts.push_back(getOrderBookMessage());
    }

    std::cout << "Order accepted: id=" << engineResult.orderId
              << " side=" << Protocol::sideToString(req.side)
              << " qty=" << req.quantity << std::endl;

    return result;
}

HandleResult RequestHandler::handleCancelOrder(const json& msg) {
    HandleResult result;

    auto req = Protocol::parseCancelOrder(msg);

    CancelResult cancelResult;
    {
        std::lock_guard<std::mutex> lock(engineMutex_);
        cancelResult = engine_.cancelOrder(req.orderId);
        metrics_.setActiveOrders(engine_.getActiveOrderCount());
    }

    if (!cancelResult.success) {
        result.directResponse = Protocol::orderRejected(cancelResult.reason);
        return result;
    }

    metrics_.recordCancel();

    result.directResponse = Protocol::orderCancelled(cancelResult.orderId);

    result.broadcasts.push_back(Protocol::orderUpdate({cancelResult.orderId, OrderStatus::CANCELLED, 0}));
    result.broadcasts.push_back(getOrderBookMessage());

    std::cout << "Order cancelled: id=" << cancelResult.orderId << std::endl;

    return result;
}

HandleResult RequestHandler::handleGetMetrics() {
    HandleResult result;
    result.directResponse = Protocol::metrics(metrics_.snapshot());
    return result;
}

json RequestHandler::getOrderBookMessage() {
    auto snapshot = engine_.getOrderBook();
    return Protocol::orderBook(snapshot);
}

} // namespace fintora
