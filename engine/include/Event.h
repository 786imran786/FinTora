#pragma once

#include <cstdint>
#include <string>
#include "Trade.h"

enum class EventType {
    ORDER_ACCEPTED,
    ORDER_CANCELLED,
    TRADE_EXECUTED,
    ORDER_PARTIALLY_FILLED,
    ORDER_COMPLETELY_FILLED,
    ORDER_BOOK_CHANGED
};

struct Event {
    EventType type;
    std::uint64_t orderId; 
    
    // For TRADE_EXECUTED
    Trade trade;
};
