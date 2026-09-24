
#include "../include/OrderBook.h"
// Includes the OrderBook class from our engine/include folder.

#include <cassert>
// Provides assert(), which stops the program if a test fails.

#include <iostream>
// Provides std::cout for printing test results.

int main()
{
    // TEST 1: EMPTY ORDER BOOK

    OrderBook book;
    // Creates an empty order book.

    assert(book.bestBid() == nullptr);
    // An empty book should have no best bid.

    assert(book.bestAsk() == nullptr);
    // An empty book should have no best ask.

    assert(book.getBidDepth(5).empty());
    // An empty book should have no bid depth.

    assert(book.getAskDepth(5).empty());
    // An empty book should have no ask depth.

    std::cout << "TEST 1 PASSED: Empty order book\n";


    // TEST 2: CREATE BUY ORDERS

    Order buy1{
        1, "AAPL", Side::BUY, OrderType::LIMIT,
        100.0, 50, 50, 1
    };
    // Order 1: BUY 50 at price 100.

    Order buy2{
        2, "AAPL", Side::BUY, OrderType::LIMIT,
        102.0, 20, 20, 2
    };
    // Order 2: BUY 20 at price 102.

    Order buy3{
        3, "AAPL", Side::BUY, OrderType::LIMIT,
        100.0, 30, 30, 3
    };
    // Order 3: BUY 30 at price 100.
    // It arrives after buy1.

    book.addOrder(buy1);
    book.addOrder(buy2);
    book.addOrder(buy3);
    // Adds all three BUY orders to the book.

    assert(book.bestBid() != nullptr);
    // A best bid should now exist.

    assert(book.bestBid()->orderId == 2);
    // Order 2 has the highest price: 102.

    assert(book.bestBid()->price == 102.0);
    // The best bid price must be 102.

    std::cout << "TEST 2 PASSED: Highest bid priority\n";


    // TEST 3: FIFO PRIORITY AT IDENTICAL PRICES

    Order buy4{
        4, "AAPL", Side::BUY, OrderType::LIMIT,
        102.0, 15, 15, 4
    };
    // Order 4 has the same price as buy2,
    // but arrives later.

    book.addOrder(buy4);
    // Adds buy4 behind buy2 at price 102.

    assert(book.bestBid()->orderId == 2);
    // buy2 must retain priority over buy4.

    std::cout << "TEST 3 PASSED: FIFO bid priority\n";


    // TEST 4: CREATE SELL ORDERS

    Order sell1{
        5, "AAPL", Side::SELL, OrderType::LIMIT,
        105.0, 40, 40, 5
    };
    // Order 5: SELL 40 at price 105.

    Order sell2{
        6, "AAPL", Side::SELL, OrderType::LIMIT,
        103.0, 25, 25, 6
    };
    // Order 6: SELL 25 at price 103.

    Order sell3{
        7, "AAPL", Side::SELL, OrderType::LIMIT,
        103.0, 10, 10, 7
    };
    // Order 7: SELL 10 at price 103.
    // It arrives after sell2.

    book.addOrder(sell1);
    book.addOrder(sell2);
    book.addOrder(sell3);
    // Adds the SELL orders to the book.

    assert(book.bestAsk() != nullptr);
    // A best ask should now exist.

    assert(book.bestAsk()->orderId == 6);
    // sell2 has the lowest price, 103,
    // and arrived before sell3.

    assert(book.bestAsk()->price == 103.0);
    // The best ask price must be 103.

    std::cout << "TEST 4 PASSED: Lowest ask and FIFO priority\n";


    // TEST 5: AGGREGATED BID L2 DEPTH

    auto bidDepth = book.getBidDepth(5);
    // Retrieves up to five bid price levels.

    assert(bidDepth.size() == 2);
    // There are two prices: 102 and 100.

    assert(bidDepth[0].first == 102.0);
    // The highest price must appear first.

    assert(bidDepth[0].second == 35);
    // At 102: buy2(20) + buy4(15) = 35.

    assert(bidDepth[1].first == 100.0);
    // The next bid price must be 100.

    assert(bidDepth[1].second == 80);
    // At 100: buy1(50) + buy3(30) = 80.

    std::cout << "TEST 5 PASSED: Aggregated bid depth\n";


    // TEST 6: AGGREGATED ASK L2 DEPTH

    auto askDepth = book.getAskDepth(5);
    // Retrieves up to five ask price levels.

    assert(askDepth.size() == 2);
    // There are two prices: 103 and 105.

    assert(askDepth[0].first == 103.0);
    // The lowest ask must appear first.

    assert(askDepth[0].second == 35);
    // At 103: sell2(25) + sell3(10) = 35.

    assert(askDepth[1].first == 105.0);
    // The next ask price must be 105.

    assert(askDepth[1].second == 40);
    // At 105: sell1(40) = 40.

    std::cout << "TEST 6 PASSED: Aggregated ask depth\n";


    // TEST 7: REQUEST ONLY ONE DEPTH LEVEL

    auto topBid = book.getBidDepth(1);
    // Requests only the best bid price level.

    auto topAsk = book.getAskDepth(1);
    // Requests only the best ask price level.

    assert(topBid.size() == 1);
    // Exactly one bid level should be returned.

    assert(topAsk.size() == 1);
    // Exactly one ask level should be returned.

    assert(topBid[0].first == 102.0);
    // The returned bid must be the highest price.

    assert(topAsk[0].first == 103.0);
    // The returned ask must be the lowest price.

    assert(book.getBidDepth(0).empty());
    // Requesting zero levels should return no bids.

    assert(book.getAskDepth(0).empty());
    // Requesting zero levels should return no asks.

    std::cout << "TEST 7 PASSED: Depth level limits\n";


    // TEST 8: PARTIALLY FILLED ORDER QUANTITY

    OrderBook partialBook;
    // Uses a separate book so previous tests are unaffected.

    Order partialBuy{
        8, "AAPL", Side::BUY, OrderType::LIMIT,
        110.0, 100, 40, 8
    };
    // Original quantity = 100.
    // Remaining quantity = 40.
    // Simulates an order that has already filled 60 units.

    partialBook.addOrder(partialBuy);
    // Adds the partially filled order.

    auto partialDepth = partialBook.getBidDepth(1);
    // Retrieves the aggregated quantity.

    assert(partialDepth.size() == 1);
    // Exactly one price level exists.

    assert(partialDepth[0].second == 40);
    // Depth must use remainingQuantity, not quantity.

    std::cout << "TEST 8 PASSED: Remaining quantity aggregation\n";


    // ALL TESTS COMPLETED

    std::cout << "\nAll Phase 2 tests passed!\n";

    return 0;
    // Returns zero to indicate successful execution.
}