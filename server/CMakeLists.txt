cmake_minimum_required(VERSION 3.16)
project(fintora-order-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---

# Boost (system, thread required by Beast/Asio)
find_package(Boost 1.74 REQUIRED COMPONENTS system thread)

# nlohmann/json — try system package first, fall back to FetchContent
find_package(nlohmann_json 3.9 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
endif()

# Threads
find_package(Threads REQUIRED)

# --- Main executable ---

add_executable(order_server
    main.cpp
    src/Protocol.cpp
    src/Metrics.cpp
    src/EngineAdapter.cpp
    src/ClientManager.cpp
    src/RequestHandler.cpp
    src/WebSocketSession.cpp
    src/Server.cpp
)

if(WIN32)
    set(PLATFORM_LIBS ws2_32 mswsock)
endif()

target_include_directories(order_server PRIVATE include)
target_link_libraries(order_server PRIVATE
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    Threads::Threads
    ${PLATFORM_LIBS}
)

# --- Tests ---

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    add_executable(protocol_test
        tests/protocol_test.cpp
        src/Protocol.cpp
        src/Metrics.cpp
        src/EngineAdapter.cpp
        src/ClientManager.cpp
        src/RequestHandler.cpp
        src/WebSocketSession.cpp
        src/Server.cpp
    )
    target_include_directories(protocol_test PRIVATE include)
    target_link_libraries(protocol_test PRIVATE
        Boost::system
        Boost::thread
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${PLATFORM_LIBS}
    )
endif()
