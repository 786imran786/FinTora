# FinTora Order Server

High-frequency limit order book WebSocket server. Acts as a thin networking/API layer around the MatchingEngine.

## Architecture

```
React Frontend
      ↓ WebSocket (JSON)
  C++ Server (this)
      ↓
  EngineAdapter
      ↓
  MatchingEngine (MEMBER 1)
      ↓
  Trade Events / Order Book
      ↓
  C++ Server (broadcast)
      ↓ WebSocket (JSON)
  React Frontend
```

### Components

| File                    | Responsibility                                       |
|-------------------------|------------------------------------------------------|
| `main.cpp`              | Entry point, port config, signal handling             |
| `include/Server.hpp`    | Accept connections, manage io_context                 |
| `include/WebSocketSession.hpp` | Per-client connection lifecycle               |
| `include/ClientManager.hpp`    | Track clients, broadcast messages              |
| `include/Protocol.hpp`  | JSON parsing, validation, serialization               |
| `include/RequestHandler.hpp`   | Route requests to engine, build responses     |
| `include/EngineAdapter.hpp`    | Adapter around MatchingEngine                 |
| `include/Metrics.hpp`   | Track orders, trades, volume, uptime                  |

## Dependencies

- **C++20** compiler (GCC 10+, Clang 12+, MSVC 2022+)
- **Boost** ≥ 1.74 (Beast, Asio, System, Thread)
- **nlohmann/json** ≥ 3.9
- **CMake** ≥ 3.16

## Setup — Linux / Ubuntu / WSL

### Install dependencies

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    nlohmann-json3-dev
```

### Build

```bash
cd server
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Run

```bash
./order_server          # default port 8080
./order_server 9090     # custom port
```

### Run tests

```bash
# Unit tests
./protocol_test

# Integration tests (server must be running)
pip install websocket-client
python ../tests/test_client.py
# or with custom host:
python ../tests/test_client.py localhost:9090
```

## Setup — Windows

### Option A: WSL (Recommended)

Follow the Linux instructions above inside WSL.

### Option B: Native Windows

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with C++ workload
2. Install [CMake](https://cmake.org/download/)
3. Install [vcpkg](https://github.com/microsoft/vcpkg):

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install boost-beast boost-asio nlohmann-json
```

4. Build:

```powershell
cd server
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

5. Run:

```powershell
.\Release\order_server.exe
```

## WebSocket Endpoint

```
ws://localhost:8080
```

## Protocol

See [protocol.md](protocol.md) for the complete JSON protocol specification.

### Quick Reference

| Request          | Response              | Broadcasts                        |
|------------------|-----------------------|-----------------------------------|
| `PLACE_ORDER`    | `ORDER_ACCEPTED`      | `TRADE`, `ORDER_UPDATE`, `ORDER_BOOK` |
| `PLACE_ORDER`    | `ORDER_REJECTED`      | —                                 |
| `CANCEL_ORDER`   | `ORDER_CANCELLED`     | `ORDER_UPDATE`, `ORDER_BOOK`      |
| `GET_METRICS`    | `METRICS`             | —                                 |
| *(invalid)*      | `ERROR`               | —                                 |

## MEMBER 1 Integration

The `EngineAdapter` class is the integration point. Currently uses a minimal stub.

**To connect the real MatchingEngine:**

1. Place engine files in `../engine/` (or adjust include paths).
2. Edit `src/EngineAdapter.cpp`:
   - Include the real MatchingEngine headers.
   - Replace stub implementations of `placeOrder()`, `cancelOrder()`, `getOrderBook()`, and `getActiveOrderCount()` with calls to the real engine.
3. Update `CMakeLists.txt` to include engine source files if needed.
4. Remove the stub state (StubOrder, orders_ map, tryMatch method) from `EngineAdapter.hpp`.

The public interface of `EngineAdapter` should remain the same — only the internal implementation changes.
