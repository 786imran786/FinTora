# FinTora — High-Frequency Matching Engine

> A high-performance, in-memory limit order book and matching engine written in C++20.  
> Built as the core component of the FinTora trading platform during a 10-hour hackathon.

---

## Table of Contents

1. [What Is This?](#what-is-this)
2. [Why Does This Exist?](#why-does-this-exist)
3. [Architecture Overview](#architecture-overview)
4. [Project Structure](#project-structure)
5. [Core Concepts](#core-concepts)
   - [What Is an Order Book?](#what-is-an-order-book)
   - [What Is Price-Time Priority?](#what-is-price-time-priority)
   - [What Is a Trade?](#what-is-a-trade)
6. [Data Models](#data-models)
   - [Order](#order)
   - [Trade](#trade)
   - [Event](#event)
7. [How the Order Book Works](#how-the-order-book-works)
   - [Two-Sided Book](#two-sided-book)
   - [Data Structure Choices](#data-structure-choices)
   - [Adding an Order](#adding-an-order)
   - [Best Bid and Best Ask](#best-bid-and-best-ask)
   - [L2 Market Depth](#l2-market-depth)
8. [How Matching Works](#how-matching-works)
   - [LIMIT Order Matching](#limit-order-matching)
   - [MARKET Order Matching](#market-order-matching)
   - [Partial Fills](#partial-fills)
   - [Multi-Level Matching](#multi-level-matching)
9. [Order Cancellation](#order-cancellation)
10. [Event System](#event-system)
11. [Public API Reference](#public-api-reference)
12. [How to Build and Run](#how-to-build-and-run)
13. [Test Suite](#test-suite)
14. [Benchmark Results](#benchmark-results)
15. [Integration Guide for Teammates](#integration-guide-for-teammates)

---

## What Is This?

This is the **core matching engine** of FinTora — the component that receives buy and sell orders, determines if any of them can be matched against each other, executes trades, and maintains a live order book of all resting (unmatched) orders.

It is purely a back-end library. It does **not** include:

- A frontend or UI
- A WebSocket server
- Authentication or user management
- A database

Those are handled by other team members who consume this engine's API.

---

## Why Does This Exist?

Every exchange — whether it's the NYSE, Binance, or a simulated trading platform — needs a matching engine at its heart. The matching engine is the piece that answers the fundamental question:

> *"When someone wants to buy and someone else wants to sell, do their prices agree? If so, make a trade."*

We built this to be:

- **Fast** — ~1.4 million orders per second on commodity hardware
- **Correct** — Follows Price-Time Priority, the industry-standard matching algorithm
- **Simple** — Clean API that a teammate can integrate in minutes
- **Self-contained** — Zero external dependencies, just C++20 STL

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                      MatchingEngine                          │
│                                                              │
│  placeOrder(order) ──► Matching Loop ──► vector<Event>       │
│  cancelOrder(id)   ──► OrderBook     ──► vector<Event>       │
│  getOrderBook()    ──► OrderBook (read-only)                 │
│  getOpenOrders()   ──► vector<Order>                         │
│  getRecentTrades() ──► vector<Trade>                         │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │                      OrderBook                         │  │
│  │                                                        │  │
│  │  Bids (BUY side)          Asks (SELL side)             │  │
│  │  ┌──────────────────┐     ┌──────────────────┐         │  │
│  │  │ 100.00 → [O1,O2] │     │ 101.00 → [O5]    │         │  │
│  │  │  99.00 → [O3]    │     │ 102.00 → [O6,O7] │         │  │
│  │  │  98.00 → [O4]    │     │ 103.00 → [O8]    │         │  │
│  │  └──────────────────┘     └──────────────────┘         │  │
│  │       ▲ highest first          ▲ lowest first          │  │
│  │                                                        │  │
│  │  orderLocations: { orderId → (side, price, iterator) } │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
engine/
├── include/                  # Header files (public interface)
│   ├── Order.h               # Order struct + Side/OrderType enums
│   ├── Trade.h               # Trade struct
│   ├── Event.h               # Event struct + EventType enum
│   ├── OrderBook.h           # OrderBook class declaration
│   └── MatchingEngine.h      # MatchingEngine class declaration
│
├── src/                      # Implementation files
│   ├── OrderBook.cpp         # OrderBook logic
│   └── MatchingEngine.cpp    # Matching logic + API
│
└── tests/                    # Tests and benchmarks
    ├── test.cpp              # Deterministic unit tests
    └── benchmark.cpp         # Performance benchmark (1M orders)
```

---

## Core Concepts

### What Is an Order Book?

An order book is a list of all active buy and sell orders for a particular asset (like a stock). It has two sides:

| Side | Name | Meaning |
|------|------|---------|
| **BUY** | Bids | "I want to buy at this price or lower" |
| **SELL** | Asks | "I want to sell at this price or higher" |

The best bid is the **highest** price someone is willing to pay.  
The best ask is the **lowest** price someone is willing to sell at.

When the best bid ≥ best ask, a trade can happen.

### What Is Price-Time Priority?

This is the algorithm that decides **which orders get matched first**. It works in two steps:

1. **Price Priority**: Better-priced orders go first.
   - For BUY orders: higher price = better (you're willing to pay more)
   - For SELL orders: lower price = better (you're willing to accept less)

2. **Time Priority**: If two orders have the **same price**, the one that arrived **earlier** goes first (FIFO — First In, First Out).

**Example:**

```
Order A: BUY 10 @ 100 (arrived at time 1)
Order B: BUY 10 @ 100 (arrived at time 2)
Order C: BUY 10 @ 101 (arrived at time 3)

Incoming: SELL 10 @ 99

Who gets matched first?
→ Order C (price 101 > 100, even though it arrived last)

If another SELL comes in:
→ Order A (same price as B, but arrived first)
```

### What Is a Trade?

A trade is the record of a successful match between a buy order and a sell order. It captures:

- Which buy order and sell order were involved
- At what price the trade happened
- How many units were exchanged
- When it happened

---

## Data Models

### Order

Defined in `include/Order.h`. Represents a single order submitted to the engine.

| Field | Type | Description |
|-------|------|-------------|
| `orderId` | `uint64_t` | Unique identifier for this order |
| `symbol` | `string` | The asset being traded (e.g., "AAPL") |
| `side` | `Side` | `BUY` or `SELL` |
| `orderType` | `OrderType` | `LIMIT` or `MARKET` |
| `price` | `double` | Desired price (ignored for MARKET orders) |
| `quantity` | `uint64_t` | Original quantity requested |
| `remainingQuantity` | `uint64_t` | How much is still unfilled |
| `timestamp` | `uint64_t` | When the order was created |

**Side enum**: `BUY`, `SELL`  
**OrderType enum**: `LIMIT`, `MARKET`

**LIMIT vs MARKET:**

- A **LIMIT** order says: "I want to trade at this specific price or better." If no match is found, the order rests in the book until someone matches it or it's cancelled.
- A **MARKET** order says: "I want to trade right now at whatever the best available price is." It never rests in the book — if it can't be filled, the unfilled portion is simply discarded.

### Trade

Defined in `include/Trade.h`. Represents one executed trade.

| Field | Type | Description |
|-------|------|-------------|
| `tradeId` | `uint64_t` | Unique identifier for this trade |
| `buyOrderId` | `uint64_t` | The buy order that was matched |
| `sellOrderId` | `uint64_t` | The sell order that was matched |
| `price` | `double` | The price at which the trade executed |
| `quantity` | `uint64_t` | How many units were exchanged |
| `timestamp` | `uint64_t` | When the trade happened |

**Important**: The trade always executes at the **resting order's price**, not the incoming order's price. This is how real exchanges work — the maker (resting order) sets the price.

### Event

Defined in `include/Event.h`. Represents something that happened in the engine. The WebSocket server layer consumes these to broadcast real-time updates.

| Field | Type | Description |
|-------|------|-------------|
| `type` | `EventType` | What kind of event this is |
| `orderId` | `uint64_t` | Which order this event relates to |
| `trade` | `Trade` | The trade details (only for `TRADE_EXECUTED`) |

**EventType enum:**

| Value | Meaning |
|-------|---------|
| `ORDER_ACCEPTED` | The engine received and acknowledged the order |
| `ORDER_CANCELLED` | The order was successfully cancelled |
| `TRADE_EXECUTED` | A trade was executed (contains full Trade details) |
| `ORDER_PARTIALLY_FILLED` | The order was partially matched but still has remaining quantity |
| `ORDER_COMPLETELY_FILLED` | The order was fully matched, nothing remains |
| `ORDER_BOOK_CHANGED` | The state of the order book changed (useful for UI updates) |

---

## How the Order Book Works

### Two-Sided Book

The `OrderBook` class (defined in `include/OrderBook.h`, implemented in `src/OrderBook.cpp`) maintains two separate sorted maps:

```cpp
// BUY side: sorted highest price → lowest price
std::map<double, std::list<Order>, std::greater<double>> bids;

// SELL side: sorted lowest price → highest price  
std::map<double, std::list<Order>> asks;
```

**Why `std::greater<double>` for bids?**  
Because the best BUY order is the one with the **highest** price. Using `std::greater` as the comparator makes `bids.begin()` point to the highest price automatically.

For asks, the default `std::less` comparator works perfectly — `asks.begin()` points to the lowest price.

### Data Structure Choices

| Structure | What | Why |
|-----------|------|-----|
| `std::map` | Price levels | Keeps prices sorted automatically. O(log N) insert/lookup. |
| `std::list<Order>` | Orders at each price | Allows O(1) removal of any order by iterator (needed for cancellation). Maintains FIFO insertion order. |
| `std::unordered_map<uint64_t, OrderLocation>` | Order ID → location | O(1) lookup to find any order instantly for cancellation. |

**Why `std::list` instead of `std::deque`?**  
We need to cancel orders in the **middle** of a price level, not just from the front. `std::list` supports O(1) erasure by iterator, while `std::deque` would require O(N) shifting. We store iterators in the `orderLocations` map so cancellation is always fast.

The `OrderLocation` struct holds:
```cpp
struct OrderLocation {
    Side side;           // Which side of the book
    double price;        // Which price level
    std::list<Order>::iterator iterator;  // Exact position in the list
};
```

### Adding an Order

When `addOrder(order)` is called:

1. Determine the side (BUY → `bids`, SELL → `asks`)
2. Append the order to the end of the list at that price level (maintains FIFO)
3. Store the iterator in `orderLocations` for fast cancellation later

```
Before: bids[100.00] = [Order1, Order2]
addOrder(Order3 @ 100.00)
After:  bids[100.00] = [Order1, Order2, Order3]
```

### Best Bid and Best Ask

- `bestBid()` → Returns a pointer to the front of the list at the highest bid price
- `bestAsk()` → Returns a pointer to the front of the list at the lowest ask price
- Returns `nullptr` if that side is empty

Both have `const` and non-`const` overloads. The non-const version lets the matching engine modify `remainingQuantity` during partial fills.

### L2 Market Depth

`getBidDepth(levels)` and `getAskDepth(levels)` return aggregated depth data.

**What is L2 depth?**  
Level 2 market data shows the total quantity available at each price level, aggregating all individual orders.

```
Individual orders:
  BUY 50 @ 100
  BUY 30 @ 100
  BUY 20 @ 100
  BUY 40 @ 99

L2 Depth (2 levels):
  [100.00, 100]    ← 50+30+20 aggregated
  [ 99.00,  40]
```

The function iterates through the sorted map, sums `remainingQuantity` for all orders at each price, and returns up to `levels` price levels.

---

## How Matching Works

All matching happens inside `MatchingEngine::placeOrder()`. Here's the complete flow:

```
placeOrder(incomingOrder)
│
├── Emit ORDER_ACCEPTED event
│
├── While incomingOrder.remainingQuantity > 0:
│   │
│   ├── If incoming is BUY:
│   │   ├── Get bestAsk() from order book
│   │   ├── If no sell orders exist → STOP matching
│   │   ├── If LIMIT and buy price < sell price → STOP matching
│   │   ├── tradeQty = min(buy remaining, sell remaining)
│   │   ├── Create Trade at the SELL order's price
│   │   ├── Reduce both orders' remainingQuantity
│   │   ├── If sell order fully filled → remove it, emit COMPLETELY_FILLED
│   │   └── If sell order partially filled → emit PARTIALLY_FILLED
│   │
│   └── If incoming is SELL:
│       ├── Get bestBid() from order book
│       ├── If no buy orders exist → STOP matching
│       ├── If LIMIT and sell price > buy price → STOP matching
│       ├── tradeQty = min(sell remaining, buy remaining)
│       ├── Create Trade at the BUY order's price
│       ├── Reduce both orders' remainingQuantity
│       ├── If buy order fully filled → remove it, emit COMPLETELY_FILLED
│       └── If buy order partially filled → emit PARTIALLY_FILLED
│
├── Emit fill status for the incoming order itself
│   ├── COMPLETELY_FILLED if remainingQuantity == 0
│   └── PARTIALLY_FILLED if some was matched but some remains
│
├── If remainingQuantity > 0 AND order is LIMIT → add to order book
│   (MARKET orders are never added to the book)
│
├── Emit ORDER_BOOK_CHANGED
│
└── Return all events
```

### LIMIT Order Matching

A LIMIT BUY at price P matches any resting SELL at price ≤ P.  
A LIMIT SELL at price P matches any resting BUY at price ≥ P.

**Example: Exact price match**
```
Book:  SELL 50 @ 100
Input: BUY 50 @ 100

100 >= 100? YES → Trade 50 @ 100
Both orders fully filled, removed from book.
```

**Example: Buy price exceeds sell price**
```
Book:  SELL 50 @ 99
Input: BUY 50 @ 101

101 >= 99? YES → Trade 50 @ 99 (at the resting SELL's price)
```

**Example: No match**
```
Book:  SELL 50 @ 102
Input: BUY 50 @ 100

100 >= 102? NO → No trade. BUY rests in book at 100.
```

### MARKET Order Matching

A MARKET order has no price restriction. It simply takes whatever is available on the opposite side.

- **MARKET BUY**: Consumes asks from lowest price upward
- **MARKET SELL**: Consumes bids from highest price downward

The key difference from LIMIT orders is:
1. The price check (`order.price < sellOrder->price`) is **skipped** for MARKET orders
2. If a MARKET order can't be fully filled, the unfilled portion is **discarded** (not added to the book)

**Example:**
```
Book:
  SELL 10 @ 100
  SELL 10 @ 101
  SELL 10 @ 102

Input: MARKET BUY qty=25

Trade 1: 10 @ 100 (consumed entirely)
Trade 2: 10 @ 101 (consumed entirely)
Trade 3:  5 @ 102 (partial fill)

Result: 3 trades, 5 remaining at SELL @ 102
MARKET order is fully filled (25 units).
```

### Partial Fills

When two orders match but have different quantities, the smaller quantity determines the trade size.

**Example:**
```
Book:  SELL 100 @ 100
Input: BUY 40 @ 100

Trade: 40 @ 100

BUY order: completely filled (removed)
SELL order: partially filled, remainingQuantity = 60 (stays in book)
```

The partially filled order **stays** in the order book. It will continue to be available for future matches.

### Multi-Level Matching

A single incoming order can match against multiple price levels.

**Example:**
```
Book:
  SELL 10 @ 101
  SELL 10 @ 102

Input: BUY 15 @ 105

Step 1: Match vs SELL @ 101 → Trade 10 @ 101. SELL fully filled.
Step 2: Match vs SELL @ 102 → Trade 5 @ 102. SELL partially filled.

Result: 2 trades, BUY fully filled, SELL @ 102 has 5 remaining.
```

---

## Order Cancellation

`cancelOrder(orderId)` removes a resting order from the book.

**How it works internally:**

1. Look up `orderId` in the `orderLocations` hash map → O(1)
2. The lookup returns the side, price, and a `std::list::iterator`
3. Erase the order from the list using the iterator → O(1)
4. If that price level is now empty, remove the entire price level from the map
5. Remove the entry from `orderLocations`

**Total time complexity: O(1)** (amortized)

If the order ID doesn't exist (already filled or never existed), `cancelOrder` returns an empty event vector — no crash, no error.

---

## Event System

Every call to `placeOrder()` or `cancelOrder()` returns a `std::vector<Event>`. These events describe everything that happened, in order.

**Example — placeOrder that results in a trade:**
```
Events returned:
1. ORDER_ACCEPTED      (orderId = incoming order)
2. TRADE_EXECUTED       (contains full Trade struct)
3. ORDER_COMPLETELY_FILLED (orderId = resting order that was consumed)
4. ORDER_COMPLETELY_FILLED (orderId = incoming order)
5. ORDER_BOOK_CHANGED
```

**Example — placeOrder with no match:**
```
Events returned:
1. ORDER_ACCEPTED
2. ORDER_BOOK_CHANGED
```

**Example — cancelOrder:**
```
Events returned:
1. ORDER_CANCELLED     (orderId = cancelled order)
2. ORDER_BOOK_CHANGED
```

The WebSocket server (built by another team member) consumes these events and broadcasts them to connected clients.

---

## Public API Reference

The `MatchingEngine` class exposes 5 methods. This is the complete public interface.

### `std::vector<Event> placeOrder(Order order)`

Submit a new order to the engine. The engine will:
1. Attempt to match it against existing resting orders
2. If unmatched quantity remains and it's a LIMIT order, add it to the book
3. Return all events that occurred

**Parameters:**
- `order` — An `Order` struct with all fields populated. The caller is responsible for assigning a unique `orderId`.

**Returns:** A vector of `Event` structs describing everything that happened.

### `std::vector<Event> cancelOrder(uint64_t orderId)`

Cancel a resting order by its ID. Returns events if successful, empty vector if order not found.

### `const OrderBook& getOrderBook() const`

Get a read-only reference to the current order book. Use this to call `getBidDepth()`, `getAskDepth()`, `bestBid()`, `bestAsk()`.

### `std::vector<Order> getOpenOrders() const`

Get a snapshot of all currently resting orders (both sides).

### `std::vector<Trade> getRecentTrades() const`

Get all trades that have been executed since the engine was created.

---

## How to Build and Run

### Prerequisites

- A C++20 compatible compiler (g++ 10+, MSVC 2019+, clang 10+)
- No external libraries required

### Compile and Run Tests

```bash
g++ -std=c++20 -O3 -I include -o test.exe src/MatchingEngine.cpp src/OrderBook.cpp tests/test.cpp
./test.exe
```

Expected output:
```
All tests passed successfully!
```

### Compile and Run Benchmark

```bash
g++ -std=c++20 -O3 -I include -o benchmark.exe src/MatchingEngine.cpp src/OrderBook.cpp tests/benchmark.cpp
./benchmark.exe
```

Expected output:
```
Generating 1000000 orders for benchmark...
Starting benchmark...
========================================
BENCHMARK RESULTS
========================================
Total Orders Processed : 1000000
Total Trades Executed  : 773442
Execution Time         : ~0.71 seconds
Orders per Second      : ~1,400,000
========================================
```

---

## Test Suite

The test suite in `tests/test.cpp` contains 9 deterministic tests:

| # | Test | What It Verifies |
|---|------|------------------|
| 1 | `testBasicLimitMatching` | Two LIMIT orders at the same price produce exactly 1 trade |
| 2 | `testNoMatchScenario` | BUY @ 99 and SELL @ 101 don't match; both rest in the book |
| 3 | `testPricePriority` | A sell matches against the higher-priced buy, not the lower one |
| 4 | `testTimePriority` | Two buys at the same price — the earlier one gets matched first |
| 5 | `testPartialFill` | BUY 40 vs SELL 100 → trade of 40, sell has 60 remaining |
| 6 | `testMultiLevelMatching` | One buy crosses two sell price levels |
| 7 | `testMarketOrders` | MARKET BUY sweeps through multiple sell levels |
| 8 | `testOrderCancellation` | Cancel an order and verify it's gone from the book |
| 9 | `testEmptyOrderBook` | MARKET order against empty book produces no trade and doesn't rest |

All tests use `assert()` — they crash immediately on failure with a clear stack trace.

---

## Benchmark Results

Measured on a standard development machine with `-O3` optimization:

| Metric | Value |
|--------|-------|
| Total Orders | 1,000,000 |
| Total Trades | 773,442 |
| Execution Time | ~0.71 seconds |
| Throughput | **~1.4 million orders/sec** |

The benchmark generates random LIMIT orders with:
- 50/50 BUY/SELL split
- Prices uniformly distributed between $90.00 and $110.00
- Quantities between 1 and 100
- Deterministic seed (42) for reproducibility

---

## Integration Guide for Teammates

### For MEMBER 2 (WebSocket Server)

Your job is to take the `std::vector<Event>` returned by `placeOrder()` and `cancelOrder()`, serialize them to JSON, and broadcast them over WebSocket.

**Minimal integration example:**

```cpp
#include "MatchingEngine.h"

MatchingEngine engine;

// When a client sends a new order:
Order order;
order.orderId = generateUniqueId();
order.symbol = "AAPL";
order.side = Side::BUY;
order.orderType = OrderType::LIMIT;
order.price = 150.0;
order.quantity = 100;
order.remainingQuantity = 100;
order.timestamp = getCurrentTimestamp();

std::vector<Event> events = engine.placeOrder(order);

// Serialize 'events' to JSON and broadcast to WebSocket clients
for (const auto& event : events) {
    // event.type tells you what happened
    // event.orderId tells you which order
    // event.trade gives you trade details (for TRADE_EXECUTED)
    broadcastToClients(serializeEvent(event));
}

// For L2 depth snapshots:
const OrderBook& book = engine.getOrderBook();
auto bids = book.getBidDepth(10);  // Top 10 bid levels
auto asks = book.getAskDepth(10);  // Top 10 ask levels
// Each entry is {price, aggregatedQuantity}
```

### Key Integration Notes

1. **Order IDs**: The engine does NOT generate order IDs. You must assign a unique `orderId` before calling `placeOrder()`.
2. **Timestamps**: Same as above — assign timestamps externally.
3. **Thread Safety**: The engine is NOT thread-safe. If you need concurrent access, wrap calls in a mutex or use a single-threaded event loop.
4. **MARKET orders**: Set `price = 0.0` (or any value — it's ignored during matching). Unfilled MARKET orders are silently discarded.
5. **Symbol field**: Currently stored but not used for routing. If you need multi-symbol support, create one `MatchingEngine` instance per symbol.
