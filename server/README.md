# FinTora Order Server

High-performance, low-latency Limit Order Book (LOB) WebSocket Server for the FinTora trading platform.

The server serves as the networking and API communication layer bridging client frontends (React/Web) with the core C++ Matching Engine. It handles high-throughput WebSocket connections, protocol validation, order routing, real-time order book snapshot distribution, trade execution broadcasts, and telemetry metrics.

---

## Table of Contents

- [System Architecture](#system-architecture)
- [End-to-End Server Workflow](#end-to-end-server-workflow)
  - [1. Server Startup & Initialization](#1-server-startup--initialization)
  - [2. Client Connection & Lifecycle](#2-client-connection--lifecycle)
  - [3. Order Placement Pipeline](#3-order-placement-pipeline)
  - [4. Order Cancellation Pipeline](#4-order-cancellation-pipeline)
  - [5. Trade Execution & Order Book Broadcast](#5-trade-execution--order-book-broadcast)
  - [6. Metrics & Performance Telemetry](#6-metrics--performance-telemetry)
  - [7. Disconnection & Cleanup](#7-disconnection--cleanup)
- [Project Structure & Component Breakdown](#project-structure--component-breakdown)
- [Concurrency & Thread Safety](#concurrency--thread-safety)
- [Message Protocol Specification](#message-protocol-specification)
- [Prerequisites & Dependencies](#prerequisites--dependencies)
- [Build Instructions](#build-instructions)
  - [Linux / Ubuntu / WSL](#linux--ubuntu--wsl)
  - [Windows (MSVC + vcpkg)](#windows-native-msvc--vcpkg)
- [Running the Server](#running-the-server)
- [Testing & Verification](#testing--verification)
  - [Unit Tests (`protocol_test`)](#1-unit-tests-protocol_test)
  - [Integration Tests (`test_client.py`)](#2-integration-tests-test_clientpy)
- [Matching Engine Integration Guide](#matching-engine-integration-guide)

---

## System Architecture

```
+-------------------------------------------------------------------------+
|                              Frontend Clients                           |
|                  (React / Web / Python Integration Test)                |
+-------------------------------------------------------------------------+
                                    ▲  │
                  WebSocket (JSON)  │  │ Requests (PLACE_ORDER, CANCEL, etc.)
        Broadcasts & Responses     │  ▼
+-------------------------------------------------------------------------+
|                           FinTora Order Server                          |
|                                                                         |
|  +---------------------+      +-------------------+                     |
|  |     Server.cpp      | ---> | WebSocketSession  | (Async read/write)  |
|  | (Boost.Asio Accept) |      +-------------------+                     |
|  +---------------------+                │                               |
|                                         ▼                               |
|                               +-------------------+                     |
|                               |  RequestHandler   |                     |
|                               +-------------------+                     |
|                                         │                               |
|                     ┌───────────────────┼──────────────────┐            |
|                     ▼                   ▼                  ▼            |
|            +-----------------+ +-----------------+ +---------------+    |
|            |  Protocol.cpp   | |  ClientManager  | |  Metrics.cpp  |    |
|            | (JSON Validate) | |   (Broadcast)   | |  (Telemetry)  |    |
|            +-----------------+ +-----------------+ +---------------+    |
|                     │                                                   |
|                     ▼                                                   |
|            +-----------------+                                          |
|            |  EngineAdapter  |                                          |
|            +-----------------+                                          |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                         Core Matching Engine                            |
|             (Price-Time Priority Order Book & Matcher)                  |
+-------------------------------------------------------------------------+
```

---

## End-to-End Server Workflow

### 1. Server Startup & Initialization
1. **Entry Point (`main.cpp`)**:
   - Parses the command-line argument for port (defaults to `8080`).
   - Instantiates shared core components:
     - `EngineAdapter`: Wraps the matching engine state and logic.
     - `Metrics`: Tracks total orders, executed trades, total traded volume, and system uptime.
     - `ClientManager`: Maintains the thread-safe set of connected WebSocket sessions.
     - `RequestHandler`: Directs incoming requests to the engine and constructs response payloads.
   - Instantiates `Server` with `boost::asio::io_context` and starts accepting TCP connections on `0.0.0.0:<port>`.
   - Configures OS signal handling (`SIGINT`, `SIGTERM`) for graceful server termination.

### 2. Client Connection & Lifecycle
```
Client                      Server (WebSocketSession)             ClientManager
  │                                     │                               │
  ├─────── TCP Connect & Handshake ────►│                               │
  │                                     ├────── Register Session ──────►│
  │                                     │                               │
  │◄────── Initial ORDER_BOOK ──────────┤ (Current snapshot sent)       │
  │                                     │                               │
  ├─────── Read Loop Started ──────────►│ (Awaits next incoming frame)  │
```
1. **Connection Acceptance**:
   - `Server::doAccept()` asynchronously accepts incoming TCP connections and spawns a `std::shared_ptr<WebSocketSession>`.
2. **WebSocket Handshake**:
   - `WebSocketSession::run()` performs an asynchronous WebSocket handshake via `boost::beast::websocket::stream`.
3. **Session Registration & Initial State Sync**:
   - Upon handshake completion, the session registers itself with `ClientManager::add()`.
   - The session queries `RequestHandler::getOrderBookMessage()` and sends the current `ORDER_BOOK` snapshot immediately to the newly connected client.
   - The session starts its continuous asynchronous message read loop (`doRead()`).

### 3. Order Placement Pipeline
```
Client                 WebSocketSession       RequestHandler        EngineAdapter         ClientManager
  │                           │                     │                     │                     │
  ├── PLACE_ORDER (JSON) ────►│                     │                     │                     │
  │                           ├── handle(rawMsg) ──►│                     │                     │
  │                           │                     ├── parse & validate ─┤                     │
  │                           │                     ├── placeOrder() ────►│                     │
  │                           │                     │                     ├── (Match / Queue)   │
  │                           │                     │◄── EngineResult ────┤                     │
  │                           │                     ├── recordMetrics()   │                     │
  │                           │◄── ORDER_ACCEPTED ──┤                     │                     │
  │◄── ORDER_ACCEPTED ────────┤   (Direct Response) │                     │                     │
  │                           │                     ├────── Broadcast Events ──────────────────►│
  │◄──────────────────────────┴─────────────────────┴────── Broadcast (TRADE, BOOK, etc.) ─────┤
```
1. **Receive & Parse**:
   - The client sends a `PLACE_ORDER` JSON frame (e.g. `{"type":"PLACE_ORDER","side":"BUY","orderType":"LIMIT","price":100,"quantity":50}`).
   - `RequestHandler::handle()` invokes `Protocol::parse()` and `Protocol::parsePlaceOrder()`.
   - If JSON format or fields are invalid, a structured `ERROR` response is generated immediately.
2. **Matching Engine Execution**:
   - `RequestHandler` acquires an engine mutex guard and calls `EngineAdapter::placeOrder()`.
   - The engine validates limits, matches opposing orders in price-time priority, fills trades, and inserts remaining limit quantities into the order book.
3. **Response & Broadcast Generation**:
   - If rejected: Client receives an `ORDER_REJECTED` direct response with reason.
   - If accepted:
     - Client receives an `ORDER_ACCEPTED` direct response with assigned `orderId`.
     - `Metrics` updates order count, trade count, volume, and active orders.
     - For every matched fill, a `TRADE` event is generated for broadcast.
     - For every status transition (FILLED / PARTIALLY_FILLED), an `ORDER_UPDATE` is generated.
     - If the order book state was altered, a refreshed `ORDER_BOOK` snapshot is queued for broadcast.
4. **Broadcast Delivery**:
   - `WebSocketSession` dispatches all queued broadcasts across all active sessions using `ClientManager::broadcast()`.

### 4. Order Cancellation Pipeline
1. Client sends `CANCEL_ORDER` with the target `orderId`.
2. `RequestHandler` parses request and calls `EngineAdapter::cancelOrder(orderId)`.
3. If order exists and is active:
   - Order is marked `CANCELLED` and removed from book.
   - Client receives direct `ORDER_CANCELLED` message.
   - An `ORDER_UPDATE` event is broadcasted.
   - Refreshed `ORDER_BOOK` is broadcasted to all connected clients.
4. If order not found or already filled, client receives an `ERROR` message (`"Order not found: <id>"`).

### 5. Trade Execution & Order Book Broadcast
- Whenever orders cross in the order book:
  - `TRADE` messages contain: `tradeId`, `price`, `quantity`, `buyerOrderId`, `sellerOrderId`, and `timestamp`.
  - `ORDER_UPDATE` messages contain: `orderId`, `status`, `filledQuantity`, `remainingQuantity`.
  - `ORDER_BOOK` messages contain aggregated top levels of bids and asks:
    ```json
    {
      "type": "ORDER_BOOK",
      "bids": [{"price": 100.0, "quantity": 50}],
      "asks": [{"price": 105.0, "quantity": 20}],
      "timestamp": 1727170000000
    }
    ```

### 6. Metrics & Performance Telemetry
- Client sends `GET_METRICS`.
- `RequestHandler` calls `Metrics::getMetrics()`, calculating:
  - `ordersProcessed`
  - `tradesExecuted`
  - `volumeTraded`
  - `activeOrders`
  - `uptimeSeconds`
- Direct `METRICS` response is returned to the requesting client.

### 7. Disconnection & Cleanup
- When a client disconnects or an error occurs on the socket:
  - `WebSocketSession` triggers `on_close` / error handler.
  - Calls `ClientManager::remove(shared_from_this())` to safely deregister the session.
  - Resource cleanups happen automatically via RAII / `std::shared_ptr`.

---

## Project Structure & Component Breakdown

```
server/
├── CMakeLists.txt                 # Modern CMake build configuration
├── main.cpp                       # Application entry point, CLI args, server initialization
├── README.md                      # Comprehensive server documentation & workflow
├── protocol.md                    # Detailed JSON protocol specification
│
├── include/
│   ├── Server.hpp                 # Async TCP listener accepting incoming connections
│   ├── WebSocketSession.hpp       # Per-client WebSocket state machine and async read/write
│   ├── ClientManager.hpp          # Thread-safe connected client registry & broadcast manager
│   ├── Protocol.hpp               # High-performance JSON parser, validator, and serializer
│   ├── RequestHandler.hpp         # Request routing, engine dispatch, and response orchestration
│   ├── EngineAdapter.hpp          # Abstraction adapter layer wrapping the core Matching Engine
│   └── Metrics.hpp                # Telemetry collector (orders, trades, volume, uptime)
│
├── src/
│   ├── Server.cpp                 # Implementation of Server connection accept loop
│   ├── WebSocketSession.cpp       # Implementation of async read/write, queue, and message dispatch
│   ├── ClientManager.cpp          # Implementation of thread-safe client management
│   ├── Protocol.cpp               # Implementation of JSON parsing & serialization functions
│   ├── RequestHandler.cpp         # Implementation of request business logic & event orchestration
│   ├── EngineAdapter.cpp          # Implementation of Matching Engine adapter and standalone stub
│   └── Metrics.cpp                # Implementation of telemetry and atomic metric counters
│
└── tests/
    ├── protocol_test.cpp          # Unit tests covering protocol parsing, validation, and JSON generation
    └── test_client.py             # Full end-to-end integration test suite via WebSocket
```

### Component Responsibility Matrix

| Component | Header / Source | Primary Responsibility |
|---|---|---|
| **Entry Point** | `main.cpp` | CLI arg parsing, signal traps (`SIGINT`/`SIGTERM`), service orchestration |
| **Server** | [Server.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/Server.hpp) / `Server.cpp` | Manages `boost::asio::ip::tcp::acceptor` and async connection listening |
| **WebSocketSession** | [WebSocketSession.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/WebSocketSession.hpp) / `WebSocketSession.cpp` | Manages Beast WebSocket stream, async read loop, write message queue, connection handshake |
| **ClientManager** | [ClientManager.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/ClientManager.hpp) / `ClientManager.cpp` | Mutex-protected set of active sessions, multi-client broadcast dispatcher |
| **Protocol** | [Protocol.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/Protocol.hpp) / `Protocol.cpp` | Schema validation, type parsing, error handling, JSON serialization using `nlohmann/json` |
| **RequestHandler** | [RequestHandler.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/RequestHandler.hpp) / `RequestHandler.cpp` | Routes incoming messages, executes engine operations, prepares response & broadcast payloads |
| **EngineAdapter** | [EngineAdapter.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/EngineAdapter.hpp) / `EngineAdapter.cpp` | Interface adapter layer between WebSocket networking and matching engine |
| **Metrics** | [Metrics.hpp](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/include/Metrics.hpp) / `Metrics.cpp` | Thread-safe atomic counters tracking runtime performance and trade statistics |

---

## Concurrency & Thread Safety

1. **Boost Beast / Asio Asynchronous IO**:
   - Connection acceptance, WebSocket handshakes, and socket reads/writes run asynchronously using `boost::asio::io_context`.
2. **Per-Session Write Queue**:
   - `WebSocketSession` maintains an internal `std::queue<std::string> writeQueue_` protected by a `std::mutex writeMutex_` to prevent concurrent asynchronous writes on the same WebSocket stream.
3. **Client Manager Synchronization**:
   - `ClientManager` protects the set of active client pointers with a `std::mutex mutex_`, ensuring safe addition, removal, and broadcast iteration across concurrent threads.
4. **Engine Adapter Thread Protection**:
   - `RequestHandler` protects access to the `EngineAdapter` with `std::mutex engineMutex_` to ensure sequential, deterministic order execution.
5. **Telemetry Synchronization**:
   - `Metrics` employs `std::mutex mutex_` / atomic access to ensure accurate, race-free counter increments across threads.

---

## Message Protocol Specification

For full details, payloads, and error codes, refer to [protocol.md](file:///c:/Users/mohdi/OneDrive/Desktop/fintora/server/protocol.md).

### Summary of Messages

| Action / Event | Type | Scope | Key Payload Fields |
|---|---|---|---|
| Place Order | `PLACE_ORDER` | Request | `side`, `orderType`, `price`, `quantity` |
| Order Accepted | `ORDER_ACCEPTED` | Direct Response | `orderId`, `timestamp` |
| Order Rejected | `ORDER_REJECTED` | Direct Response | `reason`, `timestamp` |
| Cancel Order | `CANCEL_ORDER` | Request | `orderId` |
| Order Cancelled | `ORDER_CANCELLED` | Direct Response | `orderId`, `timestamp` |
| Order Book Snapshot | `ORDER_BOOK` | On Connect & Broadcast | `bids: [...]`, `asks: [...]`, `timestamp` |
| Trade Execution | `TRADE` | Broadcast | `tradeId`, `price`, `quantity`, `buyerOrderId`, `sellerOrderId` |
| Order Status Update | `ORDER_UPDATE` | Broadcast | `orderId`, `status`, `filledQuantity`, `remainingQuantity` |
| Fetch Metrics | `GET_METRICS` | Request | *(empty)* |
| Metrics Response | `METRICS` | Direct Response | `ordersProcessed`, `tradesExecuted`, `volumeTraded`, `activeOrders`, `uptimeSeconds` |
| General Error | `ERROR` | Direct Response | `message` |

---

## Prerequisites & Dependencies

- **C++ Compiler**: Modern C++20 compliant compiler (GCC 10+, Clang 12+, MSVC 2022 v143+)
- **CMake**: Version ≥ 3.16
- **Boost Libraries**: Version ≥ 1.74 (Beast, Asio, System, Thread)
- **nlohmann/json**: Version ≥ 3.9 (JSON for Modern C++)
- **Python 3**: For integration testing (`websocket-client`)

---

## Build Instructions

### Linux / Ubuntu / WSL

```bash
# 1. Install dependencies
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev nlohmann-json3-dev

# 2. Configure and build
cd server
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Windows Native (MSVC + vcpkg)

```powershell
# 1. Install dependencies via vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install boost-beast:x64-windows boost-asio:x64-windows nlohmann-json:x64-windows

# 2. Configure and build server
cd c:\Users\mohdi\OneDrive\Desktop\fintora\server
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64
cmake --build . --config Release
```

---

## Running the Server

Start the compiled server executable (specifying an optional port, default `8080`):

### Linux / WSL:
```bash
./order_server 8080
```

### Windows:
```powershell
.\Release\order_server.exe 8080
```

**Output on launch:**
```text
=== FinTora Order Server ===
Port: 8080
Status: Running (press Ctrl+C to stop)
```

---

## Testing & Verification

### 1. Unit Tests (`protocol_test`)
Tests JSON parsing, edge-case validations, error responses, order book serialization, and trade formatting without requiring network sockets.

```bash
# In your build directory:
./protocol_test          # Linux / WSL
.\Release\protocol_test  # Windows
```

### 2. Integration Tests (`test_client.py`)
End-to-end WebSocket integration suite that connects to the live running server and tests:
- Initial `ORDER_BOOK` snapshot receipt
- Limit buy & sell order placement
- Order book price level updates
- Trade matching execution
- Order cancellation
- Metrics retrieval
- Error handling on invalid messages

```powershell
# Step 1: Ensure server is running in a separate terminal
.\order_server.exe 8080

# Step 2: In a second terminal, execute:
cd c:\Users\mohdi\OneDrive\Desktop\fintora\server\tests
pip install websocket-client
python test_client.py localhost:8080
```

**Successful Output:**
```text
============================================================
  FinTora Order Server — Integration Tests
  Target: ws://localhost:8080
============================================================

TEST 1: Connect and receive initial ORDER_BOOK
  PASSED

TEST 2: Place LIMIT BUY order
  PASSED

TEST 3: Verify ORDER_BOOK update
  PASSED

TEST 4: Place matching LIMIT SELL (Trade Execution)
  PASSED

TEST 5: Cancel an order
  PASSED

TEST 6: Get server metrics
  PASSED

TEST 7: Invalid message handling
  PASSED

============================================================
  ALL TESTS PASSED
============================================================
```

---

## Matching Engine Integration Guide

`EngineAdapter` serves as the decoupled integration boundary for the core Matching Engine.

To connect the production matching engine:

1. **Include Engine Headers**:
   Add `#include "MatchingEngine.hpp"` in `server/src/EngineAdapter.cpp`.
2. **Implement Adapter Methods**:
   Update the following four core methods in `server/src/EngineAdapter.cpp`:
   - `placeOrder(Side, OrderType, price, quantity)`
   - `cancelOrder(orderId)`
   - `getOrderBook(depth)`
   - `getActiveOrderCount()`
3. **Map Return Structures**:
   Convert the engine's internal fill events and book snapshots into `EngineResult`, `TradeEvent`, and `OrderBookSnapshot` structs defined in `server/include/EngineAdapter.hpp`.
4. **Update CMakeLists.txt**:
   Link the matching engine object files or library targets into `order_server`.
