# Matching Engine QA Test Plan

## Scope and Current Repository State

This plan validates a high-frequency limit order book and matching engine using price-time priority. The repository currently contains only placeholder files in `engine/`, `tests/`, `simulator/`, `server/`, and `frontend/`; no `Order`, `Trade`, `OrderBook`, matching API, automated tests, or `CMakeLists.txt` is available yet.

The test harness should bind the inputs below to the eventual public engine API. Where the API has not been defined, inspect results through the eventual order-book snapshot, active-order lookup, trade output, and price-level quantity APIs.

## Common Assertions

- A trade reports the correct execution price and quantity.
- Filled orders are removed from the active book; partially filled orders retain only their unfilled quantity.
- Resting orders remain ordered by price, then FIFO time priority within a price level.
- Price-level quantities equal the sum of active order quantities at that level.tests/TEST_PLAN.mdtests/TEST_PLAN.mdcd "C:\Users\janhv\OneDrive\Documents\Fintora\FinTora"

git status
git add tests/TEST_PLAN.md
git commit -m "Add matching engine QA test plan"
git push origin simulator
- A market order never becomes a resting limit order when available opposing liquidity is exhausted.
- Empty-book and non-marketable cases produce no trade and preserve valid active-order state.

## Test Cases

### TEST 1: LIMIT/LIMIT Partial Match

- **Purpose:** Validate a limit buy and limit sell matching at the same price, including a partial fill.
- **Initial state:** Empty order book.
- **Input:**
  - BUY 100 @ 100
  - SELL 50 @ 100
- **Expected output:**
  - Trade = 50 @ 100
  - BUY remaining = 50
  - SELL is fully filled and removed
- **What should be inspected:** Trade quantity and price, active BUY quantity, active SELL status, and price-level quantity at 100.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 2: Non-Marketable LIMIT/LIMIT Orders

- **Purpose:** Confirm that crossing is required for limit orders and both non-marketable orders rest in the book.
- **Initial state:** Empty order book.
- **Input:**
  - BUY 100 @ 100
  - SELL 100 @ 101
- **Expected output:**
  - No trade.
  - Both orders remain in the book.
  - BUY remains at 100 and SELL remains at 101.
- **What should be inspected:** Trade output is empty, both active orders are present, and both price-level quantities are 100.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 3: FIFO Priority at One Price

- **Purpose:** Confirm time/FIFO priority for multiple BUY orders at the same price.
- **Initial state:** Empty order book.
- **Input:**
  - BUY A 100 @ 100
  - BUY B 100 @ 100
  - SELL 150 @ 100
- **Expected output:**
  - A fills 100 first.
  - B fills 50.
  - B remaining = 50.
  - Trades occur in A-then-B order.
- **What should be inspected:** Trade participant/order IDs and sequence, A removal, B remaining quantity, and total quantity at 100.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 4: MARKET BUY Across Ask Levels

- **Purpose:** Validate MARKET BUY consumption of the best asks first across multiple price levels and a partial final fill.
- **Initial state:** Empty order book.
- **Input:**
  - SELL 50 @ 101
  - SELL 100 @ 102
  - MARKET BUY 120
- **Expected output:**
  - 50 @ 101
  - 70 @ 102
  - 30 @ 102 remains.
- **What should be inspected:** Trade sequence and prices, total market quantity filled, remaining SELL quantity at 102, and absence of a resting MARKET BUY.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 5: LIMIT Order Cancellation

- **Purpose:** Validate cancellation of an untouched resting limit order and price-level quantity maintenance.
- **Initial state:** Empty order book.
- **Input:**
  - Place LIMIT BUY 100 @ 100.
  - Cancel the order by its order ID.
- **Expected output:**
  - Order is removed from the active book and its price level quantity is updated.
  - No trade is generated.
  - Price level 100 has quantity 0 or is removed.
- **What should be inspected:** Cancellation result, active-order lookup, book snapshot, and price-level quantity.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 6: MARKET SELL Consumes Best Bids

- **Purpose:** Confirm that MARKET SELL matches the highest bid first and proceeds downward through bid levels.
- **Initial state:** Empty order book.
- **Input:**
  - BUY 40 @ 100
  - BUY 80 @ 99
  - MARKET SELL 100
- **Expected output:**
  - 40 @ 100
  - 60 @ 99
  - 20 @ 99 remains.
- **What should be inspected:** Trade sequence and prices, remaining bid quantity, and absence of a resting MARKET SELL.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 7: MARKET Order on an Empty Book

- **Purpose:** Validate safe handling of a market order when no opposing liquidity exists.
- **Initial state:** Empty order book.
- **Input:** MARKET BUY 100; repeat independently with MARKET SELL 100.
- **Expected output:**
  - No trade for either order.
  - No MARKET order remains in the active book.
  - The book remains empty.
- **What should be inspected:** Trade output, active-order count, and book snapshot after each independent run.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 8: Complete LIMIT Fill

- **Purpose:** Confirm that equal-size marketable limit orders fully fill and leave no active quantity.
- **Initial state:** Empty order book.
- **Input:** BUY 100 @ 100 followed by SELL 100 @ 100.
- **Expected output:** One trade for 100 @ 100; both orders are fully filled and removed.
- **What should be inspected:** Trade count, trade quantity and price, active-order lookup, and price-level cleanup.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 9: Non-Marketable LIMIT BUY

- **Purpose:** Confirm that a buy below the best ask rests without trading.
- **Initial state:** SELL 100 @ 105 is resting.
- **Input:** BUY 100 @ 104.
- **Expected output:** No trade; both orders remain active at their original prices.
- **What should be inspected:** Best ask, BUY price level, active quantities, and empty trade output.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 10: Non-Marketable LIMIT SELL

- **Purpose:** Confirm that a sell above the best bid rests without trading.
- **Initial state:** BUY 100 @ 100 is resting.
- **Input:** SELL 100 @ 101.
- **Expected output:** No trade; both orders remain active at their original prices.
- **What should be inspected:** Best bid, SELL price level, active quantities, and empty trade output.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 11: LIMIT Price Priority Across Ask Levels

- **Purpose:** Confirm that a marketable BUY consumes the lowest ask before higher asks, even when the higher ask arrived first.
- **Initial state:** Empty order book.
- **Input:**
  - SELL A 50 @ 102
  - SELL B 50 @ 101
  - BUY 75 @ 105
- **Expected output:**
  - 50 from SELL B @ 101 first
  - 25 from SELL A @ 102
  - SELL A remaining = 25
- **What should be inspected:** Trade order IDs, execution prices, remaining SELL A quantity, and no remaining BUY quantity.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 12: Multiple Price Levels and Full Consumption

- **Purpose:** Validate matching through several price levels with complete fills at each level.
- **Initial state:** Empty order book.
- **Input:**
  - SELL 40 @ 101
  - SELL 60 @ 102
  - BUY 100 @ 105
- **Expected output:**
  - 40 @ 101, then 60 @ 102
  - All three orders are fully filled and removed
  - No ask price levels remain
- **What should be inspected:** Trade sequence, all execution prices, active-order count, and price-level cleanup.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 13: Cancellation After Partial Fill

- **Purpose:** Confirm that a partially filled order can be cancelled and only its remaining quantity is removed.
- **Initial state:** Empty order book.
- **Input:**
  - BUY 100 @ 100 with a known order ID
  - SELL 40 @ 100
  - Cancel the partially filled BUY order
- **Expected output:**
  - Trade = 40 @ 100
  - BUY has 60 remaining after the fill, then cancellation removes that 60
  - No active quantity remains at 100
- **What should be inspected:** Partial-fill state before cancellation, cancellation result, final active-order state, and price-level quantity.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 14: FIFO Priority for SELL Orders

- **Purpose:** Confirm time/FIFO priority for multiple SELL orders at the same price.
- **Initial state:** Empty order book.
- **Input:**
  - SELL A 100 @ 100
  - SELL B 100 @ 100
  - BUY 150 @ 100
- **Expected output:**
  - A fills 100 first.
  - B fills 50.
  - B remaining = 50.
  - Trades occur in A-then-B order.
- **What should be inspected:** Trade participant/order IDs and sequence, A removal, B remaining quantity, and total quantity at 100.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

### TEST 15: Multiple Orders at the Same Price with Partial Fill

- **Purpose:** Confirm that same-price orders retain FIFO ordering after an earlier order is fully consumed and a later order is partially filled.
- **Initial state:** Empty order book.
- **Input:**
  - BUY A 25 @ 100
  - BUY B 25 @ 100
  - BUY C 25 @ 100
  - SELL 60 @ 100
- **Expected output:**
  - A fills 25, then B fills 25, then C fills 10
  - C remaining = 15
  - No A or B quantity remains
- **What should be inspected:** Trade sequence, per-order remaining quantities, FIFO queue state, and price-level quantity of 15.
- **Actual output:** _To be recorded during implementation._
- **Pass/Fail:** _Pending_

## Execution Notes

- Run each test against a fresh `OrderBook` unless the test explicitly defines a pre-populated state.
- Use deterministic order IDs and insertion order so FIFO assertions are reproducible.
- Record every trade in emitted order, including execution price, quantity, maker/taker or buy/sell order IDs when exposed by the API, and timestamp/sequence when available.
- Repeat market-order cases independently for BUY and SELL to avoid state leakage.
- Add automated tests only after the engine API and build system are present; this document is intentionally a test plan, not an implementation of those tests.
