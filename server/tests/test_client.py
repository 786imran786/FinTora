#!/usr/bin/env python3
"""
FinTora Order Server — WebSocket Test Client

Usage:
    pip install websocket-client
    python test_client.py [host:port]

Default: ws://localhost:8080
"""

import sys
import json
import time
import threading
import websocket

HOST = "ws://localhost:8080"
if len(sys.argv) > 1:
    HOST = f"ws://{sys.argv[1]}"

received = []
lock = threading.Lock()


def on_message(ws, message):
    data = json.loads(message)
    with lock:
        received.append(data)
    print(f"  <- {json.dumps(data, indent=2)}")


def on_error(ws, error):
    print(f"  !! Error: {error}")


def on_close(ws, close_status, close_msg):
    print(f"  -- Connection closed")


def on_open(ws):
    print(f"  -- Connected to {HOST}")


def send(ws, msg):
    raw = json.dumps(msg)
    print(f"  -> {raw}")
    ws.send(raw)
    time.sleep(0.3)


def wait_for(msg_type, timeout=2.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        with lock:
            for m in received:
                if m.get("type") == msg_type:
                    return m
        time.sleep(0.05)
    return None


def clear():
    with lock:
        received.clear()


def run_tests():
    print(f"\n{'='*60}")
    print(f"  FinTora Order Server — Integration Tests")
    print(f"  Target: {HOST}")
    print(f"{'='*60}\n")

    # --- Test 1: Connect and receive initial ORDER_BOOK ---
    print("TEST 1: Connect and receive initial ORDER_BOOK")
    ws = websocket.WebSocketApp(
        HOST, on_message=on_message, on_error=on_error, on_close=on_close, on_open=on_open
    )
    t = threading.Thread(target=ws.run_forever, daemon=True)
    t.start()
    time.sleep(1)

    book = wait_for("ORDER_BOOK")
    assert book is not None, "Did not receive initial ORDER_BOOK"
    assert "bids" in book and "asks" in book
    print("  PASSED\n")

    # --- Test 2: Place LIMIT BUY ---
    print("TEST 2: Place LIMIT BUY order")
    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })
    accepted = wait_for("ORDER_ACCEPTED")
    assert accepted is not None, "Did not receive ORDER_ACCEPTED"
    order_id = accepted["orderId"]
    print(f"  Order ID: {order_id}")
    print("  PASSED\n")

    # --- Test 3: Place matching LIMIT SELL → trade ---
    print("TEST 3: Place matching LIMIT SELL (expect TRADE)")
    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "SELL",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 50
    })
    time.sleep(0.5)
    trade = wait_for("TRADE")
    assert trade is not None, "Did not receive TRADE"
    assert trade["price"] == 100
    assert trade["quantity"] == 50
    print("  PASSED\n")

    # --- Test 4: Cancel order ---
    print("TEST 4: Place and cancel order")
    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 95,
        "quantity": 100
    })
    accepted = wait_for("ORDER_ACCEPTED")
    assert accepted is not None
    cancel_id = accepted["orderId"]

    clear()
    send(ws, {"type": "CANCEL_ORDER", "orderId": cancel_id})
    cancelled = wait_for("ORDER_CANCELLED")
    assert cancelled is not None, "Did not receive ORDER_CANCELLED"
    assert cancelled["orderId"] == cancel_id
    print("  PASSED\n")

    # --- Test 5: Malformed JSON ---
    print("TEST 5: Send malformed JSON")
    clear()
    ws.send("this is not json")
    time.sleep(0.3)
    error = wait_for("ERROR")
    assert error is not None, "Did not receive ERROR for malformed JSON"
    print("  PASSED\n")

    # --- Test 6: Missing fields ---
    print("TEST 6: Send request with missing fields")
    clear()
    send(ws, {"type": "PLACE_ORDER", "side": "BUY"})
    error = wait_for("ERROR")
    assert error is not None, "Did not receive ERROR for missing fields"
    print("  PASSED\n")

    # --- Test 7: Unknown message type ---
    print("TEST 7: Send unknown message type")
    clear()
    send(ws, {"type": "DESTROY_EVERYTHING"})
    error = wait_for("ERROR")
    assert error is not None, "Did not receive ERROR for unknown type"
    print("  PASSED\n")

    # --- Test 8: Get metrics ---
    print("TEST 8: Request metrics")
    clear()
    send(ws, {"type": "GET_METRICS"})
    metrics = wait_for("METRICS")
    assert metrics is not None, "Did not receive METRICS"
    assert "ordersProcessed" in metrics
    assert "tradesExecuted" in metrics
    assert "activeOrders" in metrics
    assert "ordersPerSecond" in metrics
    print(f"  Metrics: orders={metrics['ordersProcessed']}, trades={metrics['tradesExecuted']}")
    print("  PASSED\n")

    # --- Test 9: Market order ---
    print("TEST 9: Place MARKET order")
    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "SELL",
        "orderType": "LIMIT",
        "price": 105,
        "quantity": 30
    })
    time.sleep(0.3)

    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "MARKET",
        "quantity": 30
    })
    time.sleep(0.5)
    trade = wait_for("TRADE")
    assert trade is not None, "Did not receive TRADE for market order"
    assert trade["price"] == 105
    print("  PASSED\n")

    # --- Test 10: Invalid price ---
    print("TEST 10: Place order with negative price")
    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": -10,
        "quantity": 50
    })
    error = wait_for("ERROR")
    assert error is not None, "Did not receive ERROR for negative price"
    print("  PASSED\n")

    # --- Test 11: Zero quantity ---
    print("TEST 11: Place order with zero quantity")
    clear()
    send(ws, {
        "type": "PLACE_ORDER",
        "side": "BUY",
        "orderType": "LIMIT",
        "price": 100,
        "quantity": 0
    })
    error = wait_for("ERROR")
    assert error is not None, "Did not receive ERROR for zero quantity"
    print("  PASSED\n")

    # --- Done ---
    print(f"{'='*60}")
    print("  ALL INTEGRATION TESTS PASSED")
    print(f"{'='*60}")

    ws.close()


if __name__ == "__main__":
    run_tests()
