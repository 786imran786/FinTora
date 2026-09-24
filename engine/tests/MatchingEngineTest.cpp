#include "../include/MatchingEngine.h"

#include <cassert>
#include <iostream>

int main()
{
    // =========================================================
    // TEST 1: BASIC LIMIT ORDER MATCH
    // =========================================================

    MatchingEngine engine;

    // Create a BUY order:
    //
    // 100 shares @ 100
    Order buy{
        1,
        "AAPL",
        Side::BUY,
        OrderType::LIMIT,
        100.0,
        100,
        100,
        1
    };

    // Submit BUY first.
    //
    // There is no SELL order yet,
    // so the BUY should rest in the order book.
    auto firstTrades = engine.submitOrder(buy);

    assert(firstTrades.empty());

    // Create SELL:
    //
    // 50 shares @ 100
    Order sell{
        2,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        100.0,
        50,
        50,
        2
    };

    // Submit SELL.
    auto trades = engine.submitOrder(sell);

    // Exactly one trade should happen.
    assert(trades.size() == 1);

    // Trade quantity should be 50.
    assert(trades[0].quantity == 50);

    // Trade price should be 100.
    assert(trades[0].price == 100.0);

    // BUY should have 50 remaining.
    const Order* bestBid = engine.getOrderBook().bestBid();

    assert(bestBid != nullptr);
    assert(bestBid->orderId == 1);
    assert(bestBid->remainingQuantity == 50);

    // SELL should be completely filled.
    assert(engine.getOrderBook().bestAsk() == nullptr);

    std::cout << "TEST 1 PASSED: Basic limit matching\n";


    // =========================================================
    // TEST 2: MATCH ACROSS MULTIPLE PRICE LEVELS
    // =========================================================

    MatchingEngine engine2;

    // SELL @ 101
    Order sell1{
        10,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        101.0,
        20,
        20,
        1
    };

    // SELL @ 103
    Order sell2{
        11,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        103.0,
        30,
        30,
        2
    };

    // SELL @ 105
    Order sell3{
        12,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        105.0,
        40,
        40,
        3
    };

    engine2.submitOrder(sell1);
    engine2.submitOrder(sell2);
    engine2.submitOrder(sell3);

    // BUY 70 @ 105
    //
    // It should consume:
    //
    // 20 @ 101
    // 30 @ 103
    // 20 @ 105
    Order bigBuy{
        20,
        "AAPL",
        Side::BUY,
        OrderType::LIMIT,
        105.0,
        70,
        70,
        4
    };

    auto multiTrades = engine2.submitOrder(bigBuy);

    // Three price levels were matched.
    assert(multiTrades.size() == 3);

    // First trade: 20 @ 101
    assert(multiTrades[0].quantity == 20);
    assert(multiTrades[0].price == 101.0);

    // Second trade: 30 @ 103
    assert(multiTrades[1].quantity == 30);
    assert(multiTrades[1].price == 103.0);

    // Third trade: 20 @ 105
    assert(multiTrades[2].quantity == 20);
    assert(multiTrades[2].price == 105.0);

    // BUY was completely filled.
    assert(engine2.getOrderBook().bestBid() == nullptr);

    // SELL @ 105 originally had 40.
    // 20 were consumed.
    // Therefore 20 should remain.
    const Order* remainingAsk =
        engine2.getOrderBook().bestAsk();

    assert(remainingAsk != nullptr);
    assert(remainingAsk->price == 105.0);
    assert(remainingAsk->remainingQuantity == 20);

    std::cout << "TEST 2 PASSED: Multiple price-level matching\n";


    // =========================================================
    // TEST 3: NON-CROSSING ORDERS
    // =========================================================

    MatchingEngine engine3;

    // SELL 50 @ 105
    Order sell4{
        30,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        105.0,
        50,
        50,
        1
    };

    engine3.submitOrder(sell4);

    // BUY 50 @ 100
    //
    // 100 < 105
    //
    // Therefore they cannot match.
    Order buy2{
        31,
        "AAPL",
        Side::BUY,
        OrderType::LIMIT,
        100.0,
        50,
        50,
        2
    };

    auto noTrade = engine3.submitOrder(buy2);

    // No trade should happen.
    assert(noTrade.empty());

    // BUY should rest in the book.
    assert(engine3.getOrderBook().bestBid() != nullptr);

    // SELL should also remain.
    assert(engine3.getOrderBook().bestAsk() != nullptr);

    std::cout << "TEST 3 PASSED: Non-crossing orders do not match\n";


    // =========================================================
    // TEST 4: FIFO AT SAME PRICE
    // =========================================================

    MatchingEngine engine4;

    // First SELL at 100.
    Order firstSell{
        40,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        100.0,
        30,
        30,
        1
    };

    // Second SELL at the same price.
    Order secondSell{
        41,
        "AAPL",
        Side::SELL,
        OrderType::LIMIT,
        100.0,
        30,
        30,
        2
    };

    engine4.submitOrder(firstSell);
    engine4.submitOrder(secondSell);

    // BUY 40 @ 100.
    Order fifoBuy{
        42,
        "AAPL",
        Side::BUY,
        OrderType::LIMIT,
        100.0,
        40,
        40,
        3
    };

    auto fifoTrades = engine4.submitOrder(fifoBuy);

    // First SELL should be consumed completely.
    assert(fifoTrades[0].sellOrderId == 40);
    assert(fifoTrades[0].quantity == 30);

    // Then second SELL should provide remaining 10.
    assert(fifoTrades[1].sellOrderId == 41);
    assert(fifoTrades[1].quantity == 10);

    // Second SELL should have 20 remaining.
    const Order* fifoRemaining =
        engine4.getOrderBook().bestAsk();

    assert(fifoRemaining != nullptr);
    assert(fifoRemaining->orderId == 41);
    assert(fifoRemaining->remainingQuantity == 20);

    std::cout << "TEST 4 PASSED: FIFO matching at same price\n";


    std::cout << "\nAll Phase 3 tests passed!\n";

    return 0;
}