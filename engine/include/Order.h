#pragma once

#include <cstdint> // provides std::uint64_t for integer values.
#include <string>

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    //Defines possible types of orders.

    LIMIT,
    MARKET
};

class Order{
    //Represents one order submitted to the marketing engine
    public:
        std::uint64_t orderId;
        std::string symbol;

        Side side; //BUY OR SELL
        OrderType orderType; //LIMIT OR MARKET

        double price; //Price of the order.Will not be used for matching in MARKET order.

        std::uint64_t quantity; //Original quantity requested

        std::uint64_t remainingQuantity;

        std::uint64_t timestamp;

};



