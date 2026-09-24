#pragma once

#include <cstdint>

class Trade {
    public:
        std::uint64_t tradeId; //unique id of the trade

        std::uint64_t buyOrderId; // id of the buy order involved in this trade.

        std::uint64_t sellOrderId;

        double price;

        std::uint64_t quantity;

        std::uint64_t timestamp;
};

