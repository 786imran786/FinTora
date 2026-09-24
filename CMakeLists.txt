cmake_minimum_required(VERSION 3.20)

project(FinTora LANGUAGES CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fintora_engine STATIC
    engine/src/api.cpp
)

target_include_directories(fintora_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
)

if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(fintora_engine_tests
        tests/matching_engine_tests.cpp
    )

    target_link_libraries(fintora_engine_tests
        PRIVATE
            fintora_engine
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(fintora_engine_tests)
endif()