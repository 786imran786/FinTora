#include "MatchingEngine.h"

#include <algorithm>
#include <cstdint>

// =========================================================
// PLACE ORDER
// =========================================================

std::vector<Event> MatchingEngine::placeOrder(Order order)
{
    std::vector<Event> events;

    // Acknowledge order
    Event acceptedEvent;
    acceptedEvent.type = EventType::ORDER_ACCEPTED;
    acceptedEvent.orderId = order.orderId;
    events.push_back(acceptedEvent);

    while (order.remainingQuantity > 0)
    {
        if (order.side == Side::BUY)
        {
            Order* sellOrder = orderBook.bestAsk();
            if (sellOrder == nullptr)
            {
                break;
            }

            if (order.orderType == OrderType::LIMIT &&
                order.price < sellOrder->price)
            {
                break;
            }

            std::uint64_t tradeQuantity =
                std::min(order.remainingQuantity, sellOrder->remainingQuantity);

            double tradePrice = sellOrder->price;

            Trade trade;
            trade.tradeId = nextTradeId++;
            trade.buyOrderId = order.orderId;
            trade.sellOrderId = sellOrder->orderId;
            trade.price = tradePrice;
            trade.quantity = tradeQuantity;
            trade.timestamp = nextTimestamp++;

            recentTrades.push_back(trade);

            Event tradeEvent;
            tradeEvent.type = EventType::TRADE_EXECUTED;
            tradeEvent.trade = trade;
            events.push_back(tradeEvent);

            order.remainingQuantity -= tradeQuantity;
            sellOrder->remainingQuantity -= tradeQuantity;

            if (sellOrder->remainingQuantity == 0)
            {
                Event filledEvent;
                filledEvent.type = EventType::ORDER_COMPLETELY_FILLED;
                filledEvent.orderId = sellOrder->orderId;
                events.push_back(filledEvent);

                orderBook.removeBestAsk();
            }
            else
            {
                Event partialEvent;
                partialEvent.type = EventType::ORDER_PARTIALLY_FILLED;
                partialEvent.orderId = sellOrder->orderId;
                events.push_back(partialEvent);
            }
        }
        else
        {
            Order* buyOrder = orderBook.bestBid();
            if (buyOrder == nullptr)
            {
                break;
            }

            if (order.orderType == OrderType::LIMIT &&
                order.price > buyOrder->price)
            {
                break;
            }

            std::uint64_t tradeQuantity =
                std::min(order.remainingQuantity, buyOrder->remainingQuantity);

            double tradePrice = buyOrder->price;

            Trade trade;
            trade.tradeId = nextTradeId++;
            trade.buyOrderId = buyOrder->orderId;
            trade.sellOrderId = order.orderId;
            trade.price = tradePrice;
            trade.quantity = tradeQuantity;
            trade.timestamp = nextTimestamp++;

            recentTrades.push_back(trade);

            Event tradeEvent;
            tradeEvent.type = EventType::TRADE_EXECUTED;
            tradeEvent.trade = trade;
            events.push_back(tradeEvent);

            order.remainingQuantity -= tradeQuantity;
            buyOrder->remainingQuantity -= tradeQuantity;

            if (buyOrder->remainingQuantity == 0)
            {
                Event filledEvent;
                filledEvent.type = EventType::ORDER_COMPLETELY_FILLED;
                filledEvent.orderId = buyOrder->orderId;
                events.push_back(filledEvent);

                orderBook.removeBestBid();
            }
            else
            {
                Event partialEvent;
                partialEvent.type = EventType::ORDER_PARTIALLY_FILLED;
                partialEvent.orderId = buyOrder->orderId;
                events.push_back(partialEvent);
            }
        }
    }

    if (order.remainingQuantity == 0)
    {
        Event filledEvent;
        filledEvent.type = EventType::ORDER_COMPLETELY_FILLED;
        filledEvent.orderId = order.orderId;
        events.push_back(filledEvent);
    }
    else if (order.remainingQuantity < order.quantity)
    {
        Event partialEvent;
        partialEvent.type = EventType::ORDER_PARTIALLY_FILLED;
        partialEvent.orderId = order.orderId;
        events.push_back(partialEvent);
    }

    if (order.remainingQuantity > 0 && order.orderType == OrderType::LIMIT)
    {
        orderBook.addOrder(order);
    }

    Event bookEvent;
    bookEvent.type = EventType::ORDER_BOOK_CHANGED;
    events.push_back(bookEvent);

    return events;
}

// =========================================================
// CANCEL ORDER
// =========================================================

std::vector<Event> MatchingEngine::cancelOrder(std::uint64_t orderId)
{
    std::vector<Event> events;
    if (orderBook.cancelOrder(orderId))
    {
        Event cancelledEvent;
        cancelledEvent.type = EventType::ORDER_CANCELLED;
        cancelledEvent.orderId = orderId;
        events.push_back(cancelledEvent);

        Event bookEvent;
        bookEvent.type = EventType::ORDER_BOOK_CHANGED;
        events.push_back(bookEvent);
    }
    return events;
}

// =========================================================
// GET ORDER BOOK
// =========================================================

const OrderBook& MatchingEngine::getOrderBook() const
{
    return orderBook;
}

// =========================================================
// GET OPEN ORDERS
// =========================================================

std::vector<Order> MatchingEngine::getOpenOrders() const
{
    return orderBook.getOpenOrders();
}

// =========================================================
// GET RECENT TRADES
// =========================================================

std::vector<Trade> MatchingEngine::getRecentTrades() const
{
    return recentTrades;
}