#include <iostream>
#include <cassert>
#include "../include/MatchingEngine.h"

void testBasicLimitMatching() {
    MatchingEngine engine;
    Order buyOrder{1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 10, 1};
    Order sellOrder{2, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 10, 2};

    auto events1 = engine.placeOrder(buyOrder);
    assert(events1.size() == 2); // ACCEPTED, BOOK_CHANGED

    auto events2 = engine.placeOrder(sellOrder);
    // ACCEPTED, TRADE, COMPLETELY_FILLED (buy), COMPLETELY_FILLED (sell), BOOK_CHANGED
    assert(events2.size() == 5); 
    assert(engine.getRecentTrades().size() == 1);
    assert(engine.getOpenOrders().empty());
}

void testNoMatchScenario() {
    MatchingEngine engine;
    Order buyOrder{1, "AAPL", Side::BUY, OrderType::LIMIT, 99.0, 10, 10, 1};
    Order sellOrder{2, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 10, 10, 2};

    engine.placeOrder(buyOrder);
    engine.placeOrder(sellOrder);

    assert(engine.getRecentTrades().empty());
    assert(engine.getOpenOrders().size() == 2);
}

void testPricePriority() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::BUY, OrderType::LIMIT, 99.0, 10, 10, 1});
    engine.placeOrder({2, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 10, 2});

    engine.placeOrder({3, "AAPL", Side::SELL, OrderType::LIMIT, 98.0, 10, 10, 3});

    auto trades = engine.getRecentTrades();
    assert(trades.size() == 1);
    assert(trades[0].buyOrderId == 2); // Matched with the better priced buy order
}

void testTimePriority() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 10, 1});
    engine.placeOrder({2, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 10, 2});

    engine.placeOrder({3, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 10, 3});

    auto trades = engine.getRecentTrades();
    assert(trades.size() == 1);
    assert(trades[0].buyOrderId == 1); // Matched with the earlier order
}

void testPartialFill() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 40, 40, 1});
    engine.placeOrder({2, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 100, 100, 2});

    auto trades = engine.getRecentTrades();
    assert(trades.size() == 1);
    assert(trades[0].quantity == 40);

    auto openOrders = engine.getOpenOrders();
    assert(openOrders.size() == 1);
    assert(openOrders[0].remainingQuantity == 60);
}

void testMultiLevelMatching() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 10, 10, 1});
    engine.placeOrder({2, "AAPL", Side::SELL, OrderType::LIMIT, 102.0, 10, 10, 2});

    engine.placeOrder({3, "AAPL", Side::BUY, OrderType::LIMIT, 105.0, 15, 15, 3});

    auto trades = engine.getRecentTrades();
    assert(trades.size() == 2);
    assert(trades[0].price == 101.0);
    assert(trades[0].quantity == 10);
    assert(trades[1].price == 102.0);
    assert(trades[1].quantity == 5);
}

void testMarketOrders() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::SELL, OrderType::LIMIT, 100.0, 10, 10, 1});
    engine.placeOrder({2, "AAPL", Side::SELL, OrderType::LIMIT, 101.0, 10, 10, 2});

    // Market BUY should cross both sellers
    engine.placeOrder({3, "AAPL", Side::BUY, OrderType::MARKET, 0.0, 15, 15, 3});

    auto trades = engine.getRecentTrades();
    assert(trades.size() == 2);
    assert(trades[0].quantity == 10);
    assert(trades[1].quantity == 5);

    auto openOrders = engine.getOpenOrders();
    assert(openOrders.size() == 1); // Order 2 has 5 left
    assert(openOrders[0].remainingQuantity == 5);
}

void testOrderCancellation() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::BUY, OrderType::LIMIT, 100.0, 10, 10, 1});
    assert(engine.getOpenOrders().size() == 1);

    auto events = engine.cancelOrder(1);
    assert(events.size() == 2); // CANCELLED, BOOK_CHANGED
    assert(engine.getOpenOrders().empty());
}

void testEmptyOrderBook() {
    MatchingEngine engine;
    engine.placeOrder({1, "AAPL", Side::BUY, OrderType::MARKET, 0.0, 10, 10, 1});
    assert(engine.getRecentTrades().empty());
    assert(engine.getOpenOrders().empty()); // Market order shouldn't rest
}

int main() {
    testBasicLimitMatching();
    testNoMatchScenario();
    testPricePriority();
    testTimePriority();
    testPartialFill();
    testMultiLevelMatching();
    testMarketOrders();
    testOrderCancellation();
    testEmptyOrderBook();

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
