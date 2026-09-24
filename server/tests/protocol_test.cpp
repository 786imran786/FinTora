#include "Protocol.hpp"
#include "Metrics.hpp"
#include "EngineAdapter.hpp"
#include "RequestHandler.hpp"
#include "ClientManager.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#define TEST(name) \
    std::cout << "TEST: " << #name << "... "; \
    name(); \
    std::cout << "PASSED" << std::endl;

using namespace fintora;

void testParseValidJson() {
    auto msg = Protocol::parse(R"({"type":"PLACE_ORDER"})");
    assert(msg["type"] == "PLACE_ORDER");
}

void testParseInvalidJson() {
    try {
        Protocol::parse("not json");
        assert(false);
    } catch (const ProtocolError&) {}
}

void testParseEmptyMessage() {
    try {
        Protocol::parse("");
        assert(false);
    } catch (const ProtocolError&) {}
}

void testGetMessageType() {
    auto msg = Protocol::parse(R"({"type":"CANCEL_ORDER"})");
    assert(Protocol::getMessageType(msg) == "CANCEL_ORDER");
}

void testMissingMessageType() {
    auto msg = Protocol::parse(R"({"side":"BUY"})");
    try {
        Protocol::getMessageType(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testParseLimitBuyOrder() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");
    auto req = Protocol::parsePlaceOrder(msg);
    assert(req.side == Side::BUY);
    assert(req.orderType == OrderType::LIMIT);
    assert(req.price == 100.0);
    assert(req.quantity == 50);
}

void testParseLimitSellOrder() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "SELL",
        "orderType": "LIMIT",
        "price": 101.5,
        "quantity": 25
    })");
    auto req = Protocol::parsePlaceOrder(msg);
    assert(req.side == Side::SELL);
    assert(req.orderType == OrderType::LIMIT);
    assert(req.price == 101.5);
    assert(req.quantity == 25);
}

void testParseMarketOrder() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "MARKET",
        "quantity": 100
    })");
    auto req = Protocol::parsePlaceOrder(msg);
    assert(req.side == Side::BUY);
    assert(req.orderType == OrderType::MARKET);
    assert(req.quantity == 100);
}

void testMissingSide() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testInvalidSide() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "HOLD",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testMissingQuantity() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testZeroQuantity() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 0
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testNegativeQuantity() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": -10
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testMissingPriceForLimit() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "quantity": 50
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testInvalidOrderType() {
    auto msg = Protocol::parse(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "STOP",
        "price": 100,
        "quantity": 50
    })");
    try {
        Protocol::parsePlaceOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testParseCancelOrder() {
    auto msg = Protocol::parse(R"({"type":"CANCEL_ORDER","orderId":42})");
    auto req = Protocol::parseCancelOrder(msg);
    assert(req.orderId == 42);
}

void testMissingOrderId() {
    auto msg = Protocol::parse(R"({"type":"CANCEL_ORDER"})");
    try {
        Protocol::parseCancelOrder(msg);
        assert(false);
    } catch (const ProtocolError&) {}
}

void testOrderAcceptedResponse() {
    auto j = Protocol::orderAccepted(42);
    assert(j["type"] == "ORDER_ACCEPTED");
    assert(j["orderId"] == 42);
}

void testOrderRejectedResponse() {
    auto j = Protocol::orderRejected("Invalid quantity");
    assert(j["type"] == "ORDER_REJECTED");
    assert(j["reason"] == "Invalid quantity");
}

void testOrderCancelledResponse() {
    auto j = Protocol::orderCancelled(99);
    assert(j["type"] == "ORDER_CANCELLED");
    assert(j["orderId"] == 99);
}

void testTradeEventResponse() {
    TradeEvent trade{1, 100.0, 50};
    auto j = Protocol::tradeEvent(trade);
    assert(j["type"] == "TRADE");
    assert(j["tradeId"] == 1);
    assert(j["price"] == 100.0);
    assert(j["quantity"] == 50);
}

void testOrderUpdateResponse() {
    OrderUpdateEvent update{42, OrderStatus::PARTIALLY_FILLED, 25};
    auto j = Protocol::orderUpdate(update);
    assert(j["type"] == "ORDER_UPDATE");
    assert(j["orderId"] == 42);
    assert(j["status"] == "PARTIALLY_FILLED");
    assert(j["remainingQuantity"] == 25);
}

void testOrderBookResponse() {
    OrderBookSnapshot snapshot;
    snapshot.bids = {{100.0, 300}, {99.0, 150}};
    snapshot.asks = {{101.0, 200}, {102.0, 400}};
    auto j = Protocol::orderBook(snapshot);
    assert(j["type"] == "ORDER_BOOK");
    assert(j["bids"].size() == 2);
    assert(j["asks"].size() == 2);
    assert(j["bids"][0][0] == 100.0);
    assert(j["bids"][0][1] == 300);
    assert(j["asks"][0][0] == 101.0);
    assert(j["asks"][0][1] == 200);
}

void testEmptyOrderBook() {
    OrderBookSnapshot snapshot;
    auto j = Protocol::orderBook(snapshot);
    assert(j["bids"].empty());
    assert(j["asks"].empty());
}

void testMetricsResponse() {
    MetricsData data{100, 50, 10, 5000, 33.3};
    auto j = Protocol::metrics(data);
    assert(j["type"] == "METRICS");
    assert(j["ordersProcessed"] == 100);
    assert(j["tradesExecuted"] == 50);
    assert(j["activeOrders"] == 10);
    assert(j["totalVolume"] == 5000);
}

void testErrorResponse() {
    auto j = Protocol::error("Something went wrong");
    assert(j["type"] == "ERROR");
    assert(j["message"] == "Something went wrong");
}

void testMetricsTracking() {
    Metrics m;
    m.recordOrder();
    m.recordOrder();
    m.recordTrade(100);
    m.setActiveOrders(5);

    auto snap = m.snapshot();
    assert(snap.ordersProcessed == 2);
    assert(snap.tradesExecuted == 1);
    assert(snap.activeOrders == 5);
    assert(snap.totalVolume == 100);
    assert(snap.ordersPerSecond >= 0.0);
}

void testEngineAdapterPlaceOrder() {
    EngineAdapter engine;
    auto result = engine.placeOrder(Side::BUY, OrderType::LIMIT, 100.0, 50);
    assert(result.accepted);
    assert(result.orderId == 1);
    assert(result.trades.empty());
}

void testEngineAdapterMatchingOrders() {
    EngineAdapter engine;
    auto buy = engine.placeOrder(Side::BUY, OrderType::LIMIT, 100.0, 50);
    assert(buy.accepted);
    assert(buy.trades.empty());

    auto sell = engine.placeOrder(Side::SELL, OrderType::LIMIT, 100.0, 50);
    assert(sell.accepted);
    assert(sell.trades.size() == 1);
    assert(sell.trades[0].price == 100.0);
    assert(sell.trades[0].quantity == 50);
}

void testEngineAdapterPartialFill() {
    EngineAdapter engine;
    engine.placeOrder(Side::BUY, OrderType::LIMIT, 100.0, 30);
    auto sell = engine.placeOrder(Side::SELL, OrderType::LIMIT, 100.0, 50);

    assert(sell.accepted);
    assert(sell.trades.size() == 1);
    assert(sell.trades[0].quantity == 30);
}

void testEngineAdapterCancel() {
    EngineAdapter engine;
    auto result = engine.placeOrder(Side::BUY, OrderType::LIMIT, 100.0, 50);
    auto cancel = engine.cancelOrder(result.orderId);
    assert(cancel.success);
    assert(cancel.orderId == result.orderId);
}

void testEngineAdapterCancelNonexistent() {
    EngineAdapter engine;
    auto cancel = engine.cancelOrder(999);
    assert(!cancel.success);
}

void testEngineAdapterOrderBook() {
    EngineAdapter engine;
    engine.placeOrder(Side::BUY, OrderType::LIMIT, 100.0, 50);
    engine.placeOrder(Side::BUY, OrderType::LIMIT, 99.0, 30);
    engine.placeOrder(Side::SELL, OrderType::LIMIT, 101.0, 25);

    auto book = engine.getOrderBook();
    assert(book.bids.size() == 2);
    assert(book.asks.size() == 1);
    assert(book.bids[0].price == 100.0);
    assert(book.bids[1].price == 99.0);
    assert(book.asks[0].price == 101.0);
}

void testEngineAdapterMarketOrder() {
    EngineAdapter engine;
    engine.placeOrder(Side::SELL, OrderType::LIMIT, 101.0, 50);
    auto result = engine.placeOrder(Side::BUY, OrderType::MARKET, 0.0, 25);

    assert(result.accepted);
    assert(result.trades.size() == 1);
    assert(result.trades[0].price == 101.0);
    assert(result.trades[0].quantity == 25);
}

void testEngineAdapterMarketOrderNoLiquidity() {
    EngineAdapter engine;
    auto result = engine.placeOrder(Side::BUY, OrderType::MARKET, 0.0, 50);
    assert(!result.accepted);
}

void testRequestHandlerPlaceOrder() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    auto result = handler.handle(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");

    assert(result.directResponse["type"] == "ORDER_ACCEPTED");
    assert(result.directResponse["orderId"] == 1);
}

void testRequestHandlerInvalidJson() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    auto result = handler.handle("not json at all");
    assert(result.directResponse["type"] == "ERROR");
}

void testRequestHandlerUnknownType() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    auto result = handler.handle(R"({"type":"UNKNOWN_TYPE"})");
    assert(result.directResponse["type"] == "ERROR");
}

void testRequestHandlerMissingFields() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    auto result = handler.handle(R"({"type":"PLACE_ORDER","side":"BUY"})");
    assert(result.directResponse["type"] == "ERROR");
}

void testRequestHandlerCancel() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    handler.handle(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");

    auto result = handler.handle(R"({"type":"CANCEL_ORDER","orderId":1})");
    assert(result.directResponse["type"] == "ORDER_CANCELLED");
    assert(result.directResponse["orderId"] == 1);
}

void testRequestHandlerGetMetrics() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    auto result = handler.handle(R"({"type":"GET_METRICS"})");
    assert(result.directResponse["type"] == "METRICS");
    assert(result.directResponse.contains("ordersProcessed"));
    assert(result.directResponse.contains("tradesExecuted"));
    assert(result.directResponse.contains("activeOrders"));
    assert(result.directResponse.contains("totalVolume"));
    assert(result.directResponse.contains("ordersPerSecond"));
}

void testRequestHandlerTradeGeneration() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    handler.handle(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");

    auto result = handler.handle(R"({
        "type": "PLACE_ORDER",
        "side": "SELL",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })");

    assert(result.directResponse["type"] == "ORDER_ACCEPTED");

    bool hasTrade = false;
    bool hasOrderBook = false;
    bool hasOrderUpdate = false;
    for (const auto& broadcast : result.broadcasts) {
        if (broadcast["type"] == "TRADE") hasTrade = true;
        if (broadcast["type"] == "ORDER_BOOK") hasOrderBook = true;
        if (broadcast["type"] == "ORDER_UPDATE") hasOrderUpdate = true;
    }
    assert(hasTrade);
    assert(hasOrderBook);
    assert(hasOrderUpdate);
}

void testRequestHandlerNegativePrice() {
    EngineAdapter engine;
    Metrics metrics;
    ClientManager clients;
    RequestHandler handler(engine, metrics, clients);

    auto result = handler.handle(R"({
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": -5,
        "quantity": 50
    })");
    assert(result.directResponse["type"] == "ERROR");
}

int main() {
    std::cout << "=== Protocol Tests ===" << std::endl;
    TEST(testParseValidJson);
    TEST(testParseInvalidJson);
    TEST(testParseEmptyMessage);
    TEST(testGetMessageType);
    TEST(testMissingMessageType);
    TEST(testParseLimitBuyOrder);
    TEST(testParseLimitSellOrder);
    TEST(testParseMarketOrder);
    TEST(testMissingSide);
    TEST(testInvalidSide);
    TEST(testMissingQuantity);
    TEST(testZeroQuantity);
    TEST(testNegativeQuantity);
    TEST(testMissingPriceForLimit);
    TEST(testInvalidOrderType);
    TEST(testParseCancelOrder);
    TEST(testMissingOrderId);

    std::cout << "\n=== Response Construction Tests ===" << std::endl;
    TEST(testOrderAcceptedResponse);
    TEST(testOrderRejectedResponse);
    TEST(testOrderCancelledResponse);
    TEST(testTradeEventResponse);
    TEST(testOrderUpdateResponse);
    TEST(testOrderBookResponse);
    TEST(testEmptyOrderBook);
    TEST(testMetricsResponse);
    TEST(testErrorResponse);

    std::cout << "\n=== Metrics Tests ===" << std::endl;
    TEST(testMetricsTracking);

    std::cout << "\n=== EngineAdapter Tests ===" << std::endl;
    TEST(testEngineAdapterPlaceOrder);
    TEST(testEngineAdapterMatchingOrders);
    TEST(testEngineAdapterPartialFill);
    TEST(testEngineAdapterCancel);
    TEST(testEngineAdapterCancelNonexistent);
    TEST(testEngineAdapterOrderBook);
    TEST(testEngineAdapterMarketOrder);
    TEST(testEngineAdapterMarketOrderNoLiquidity);

    std::cout << "\n=== RequestHandler Tests ===" << std::endl;
    TEST(testRequestHandlerPlaceOrder);
    TEST(testRequestHandlerInvalidJson);
    TEST(testRequestHandlerUnknownType);
    TEST(testRequestHandlerMissingFields);
    TEST(testRequestHandlerCancel);
    TEST(testRequestHandlerGetMetrics);
    TEST(testRequestHandlerTradeGeneration);
    TEST(testRequestHandlerNegativePrice);

    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    return 0;
}
