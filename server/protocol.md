# FinTora WebSocket Protocol

## Overview

The FinTora order server communicates with clients via WebSocket using JSON text messages.

- **Transport**: WebSocket (RFC 6455)
- **Format**: JSON text frames
- **Default endpoint**: `ws://localhost:8080`

## Connection Behavior

1. Client opens a WebSocket connection to the server.
2. Server immediately sends the current `ORDER_BOOK` snapshot.
3. Client sends request messages; server responds with direct responses and broadcasts.
4. When the order book changes, the server broadcasts `ORDER_BOOK` to **all** connected clients.
5. When a trade occurs, the server broadcasts `TRADE` to **all** connected clients.
6. When an order status changes, the server broadcasts `ORDER_UPDATE` to **all** connected clients.

### Message Delivery

| Message Type      | Delivery       | Description                                |
|-------------------|----------------|--------------------------------------------|
| `ORDER_ACCEPTED`  | Direct (sender)| Confirmation that order was accepted        |
| `ORDER_REJECTED`  | Direct (sender)| Order was rejected with a reason            |
| `ORDER_CANCELLED` | Direct (sender)| Confirmation that order was cancelled       |
| `ERROR`           | Direct (sender)| Validation or processing error              |
| `METRICS`         | Direct (sender)| Response to `GET_METRICS` request           |
| `TRADE`           | Broadcast (all)| A trade was executed                        |
| `ORDER_UPDATE`    | Broadcast (all)| An order's status changed                   |
| `ORDER_BOOK`      | Broadcast (all)| The order book state changed                |

---

## Request Messages (Client → Server)

### 1. PLACE_ORDER — Limit

Place a limit order at a specific price.

```json
{
  "type": "PLACE_ORDER",
  "side": "BUY",
  "orderType": "LIMIT",
  "price": 100,
  "quantity": 50
}
```

| Field       | Type   | Required | Valid Values         |
|-------------|--------|----------|----------------------|
| `type`      | string | Yes      | `"PLACE_ORDER"`      |
| `side`      | string | Yes      | `"BUY"`, `"SELL"`    |
| `orderType` | string | Yes      | `"LIMIT"`            |
| `price`     | number | Yes      | `> 0`                |
| `quantity`  | number | Yes      | `> 0` (integer)      |

### 2. PLACE_ORDER — Market

Place a market order (executes at best available price).

```json
{
  "type": "PLACE_ORDER",
  "side": "BUY",
  "orderType": "MARKET",
  "quantity": 50
}
```

| Field       | Type   | Required | Valid Values         |
|-------------|--------|----------|----------------------|
| `type`      | string | Yes      | `"PLACE_ORDER"`      |
| `side`      | string | Yes      | `"BUY"`, `"SELL"`    |
| `orderType` | string | Yes      | `"MARKET"`           |
| `price`     | number | No       | Ignored for market   |
| `quantity`  | number | Yes      | `> 0` (integer)      |

### 3. CANCEL_ORDER

Cancel an existing order by its ID.

```json
{
  "type": "CANCEL_ORDER",
  "orderId": 123
}
```

| Field     | Type   | Required | Description          |
|-----------|--------|----------|----------------------|
| `type`    | string | Yes      | `"CANCEL_ORDER"`     |
| `orderId` | number | Yes      | ID from `ORDER_ACCEPTED` |

### 4. GET_METRICS

Request current server metrics.

```json
{
  "type": "GET_METRICS"
}
```

| Field  | Type   | Required | Valid Values     |
|--------|--------|----------|------------------|
| `type` | string | Yes      | `"GET_METRICS"`  |

---

## Response/Event Messages (Server → Client)

### 5. ORDER_ACCEPTED

Sent to the requesting client when an order is accepted.

```json
{
  "type": "ORDER_ACCEPTED",
  "orderId": 123
}
```

| Field     | Type   | Description                     |
|-----------|--------|---------------------------------|
| `type`    | string | Always `"ORDER_ACCEPTED"`       |
| `orderId` | number | Server-assigned order ID        |

### 6. ORDER_REJECTED

Sent to the requesting client when an order is rejected.

```json
{
  "type": "ORDER_REJECTED",
  "reason": "Invalid quantity"
}
```

| Field    | Type   | Description                     |
|----------|--------|---------------------------------|
| `type`   | string | Always `"ORDER_REJECTED"`       |
| `reason` | string | Human-readable rejection reason |

### 7. ORDER_CANCELLED

Sent to the requesting client when an order is successfully cancelled.

```json
{
  "type": "ORDER_CANCELLED",
  "orderId": 123
}
```

| Field     | Type   | Description                     |
|-----------|--------|---------------------------------|
| `type`    | string | Always `"ORDER_CANCELLED"`      |
| `orderId` | number | ID of the cancelled order       |

### 8. TRADE

Broadcast to **all** connected clients when a trade is executed.

```json
{
  "type": "TRADE",
  "tradeId": 123,
  "price": 100,
  "quantity": 50
}
```

| Field      | Type   | Description               |
|------------|--------|---------------------------|
| `type`     | string | Always `"TRADE"`          |
| `tradeId`  | number | Unique trade identifier   |
| `price`    | number | Execution price           |
| `quantity` | number | Executed quantity         |

### 9. ORDER_UPDATE

Broadcast to **all** connected clients when an order's status changes.

```json
{
  "type": "ORDER_UPDATE",
  "orderId": 123,
  "status": "PARTIALLY_FILLED",
  "remainingQuantity": 50
}
```

| Field               | Type   | Description                          |
|---------------------|--------|--------------------------------------|
| `type`              | string | Always `"ORDER_UPDATE"`              |
| `orderId`           | number | ID of the affected order             |
| `status`            | string | See status values below              |
| `remainingQuantity` | number | Remaining unfilled quantity          |

**Status values:**

| Status              | Meaning                                    |
|---------------------|--------------------------------------------|
| `ACCEPTED`          | Order accepted and resting on the book     |
| `PARTIALLY_FILLED`  | Order partially filled, remainder resting  |
| `FILLED`            | Order completely filled                    |
| `CANCELLED`         | Order cancelled                            |

### 10. ORDER_BOOK

Broadcast to **all** connected clients when the order book changes. Also sent to a new client on connection.

```json
{
  "type": "ORDER_BOOK",
  "bids": [
    [100, 300],
    [99, 150]
  ],
  "asks": [
    [101, 200],
    [102, 400]
  ]
}
```

| Field  | Type           | Description                                   |
|--------|----------------|-----------------------------------------------|
| `type` | string         | Always `"ORDER_BOOK"`                         |
| `bids` | array of arrays| Bid levels `[price, quantity]`, highest first |
| `asks` | array of arrays| Ask levels `[price, quantity]`, lowest first  |

This is **L2 aggregated** market depth. Each level represents the total quantity at that price.

### 11. ERROR

Sent to the requesting client for invalid requests.

```json
{
  "type": "ERROR",
  "message": "Invalid request"
}
```

| Field     | Type   | Description                       |
|-----------|--------|-----------------------------------|
| `type`    | string | Always `"ERROR"`                  |
| `message` | string | Human-readable error description  |

**Error conditions:**
- Invalid JSON
- Empty message
- Missing `type` field
- Unknown message type
- Invalid `side` (not `"BUY"` or `"SELL"`)
- Invalid `orderType` (not `"LIMIT"` or `"MARKET"`)
- Missing `quantity`
- `quantity <= 0`
- Missing `price` for LIMIT orders
- `price <= 0` for LIMIT orders
- Missing `orderId` for CANCEL_ORDER
- Engine errors

### 12. METRICS

Sent to the requesting client in response to `GET_METRICS`.

```json
{
  "type": "METRICS",
  "ordersProcessed": 10000,
  "tradesExecuted": 4500,
  "activeOrders": 1000,
  "totalVolume": 500000,
  "ordersPerSecond": 5000
}
```

| Field              | Type   | Description                                    |
|--------------------|--------|------------------------------------------------|
| `type`             | string | Always `"METRICS"`                             |
| `ordersProcessed`  | number | Total orders processed since server start      |
| `tradesExecuted`   | number | Total trades executed since server start       |
| `activeOrders`     | number | Currently resting orders on the book           |
| `totalVolume`      | number | Sum of all traded quantities                   |
| `ordersPerSecond`  | number | `ordersProcessed / elapsed_seconds`            |

All values are derived from actual server activity. No values are fabricated.

---

## Event Sequence

### Successful Limit Order (no match)

```
Client → PLACE_ORDER (LIMIT)
Client ← ORDER_ACCEPTED
All    ← ORDER_BOOK
```

### Successful Limit Order (with match)

```
Client → PLACE_ORDER (LIMIT)
Client ← ORDER_ACCEPTED
All    ← TRADE (for each fill)
All    ← ORDER_UPDATE (for each affected order)
All    ← ORDER_BOOK
```

### Successful Market Order

```
Client → PLACE_ORDER (MARKET)
Client ← ORDER_ACCEPTED
All    ← TRADE (for each fill)
All    ← ORDER_UPDATE (for each affected order)
All    ← ORDER_BOOK
```

### Order Cancellation

```
Client → CANCEL_ORDER
Client ← ORDER_CANCELLED
All    ← ORDER_UPDATE (status: CANCELLED)
All    ← ORDER_BOOK
```

### Invalid Request

```
Client → (invalid message)
Client ← ERROR
```

---

## Architecture Notes

- **Price-time priority matching** is handled entirely by the MatchingEngine (MEMBER 1's component).
- The server is a thin networking/API layer — it does not implement matching logic.
- All trades and order state transitions come from the MatchingEngine.
- The server serializes access to the MatchingEngine for thread safety.
