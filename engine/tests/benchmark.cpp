#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include "../include/MatchingEngine.h"

int main() {
    MatchingEngine engine;
    
    const int NUM_ORDERS = 1000000;
    std::cout << "Generating " << NUM_ORDERS << " orders for benchmark..." << std::endl;

    std::vector<Order> orders;
    orders.reserve(NUM_ORDERS);

    std::mt19937 gen(42);
    std::uniform_int_distribution<> side_dist(0, 1);
    std::uniform_real_distribution<> price_dist(90.0, 110.0);
    std::uniform_int_distribution<> qty_dist(1, 100);

    for (int i = 0; i < NUM_ORDERS; ++i) {
        Side side = (side_dist(gen) == 0) ? Side::BUY : Side::SELL;
        double price = price_dist(gen);
        // Round to 2 decimal places to simulate realistic ticks
        price = std::round(price * 100.0) / 100.0;
        uint64_t qty = qty_dist(gen);

        orders.push_back({
            static_cast<uint64_t>(i + 1),
            "AAPL",
            side,
            OrderType::LIMIT,
            price,
            qty,
            qty,
            static_cast<uint64_t>(i + 1)
        });
    }

    std::cout << "Starting benchmark..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& order : orders) {
        engine.placeOrder(order);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end_time - start_time;
    double execution_time = diff.count();
    
    auto trades = engine.getRecentTrades();

    std::cout << "========================================" << std::endl;
    std::cout << "BENCHMARK RESULTS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total Orders Processed : " << NUM_ORDERS << std::endl;
    std::cout << "Total Trades Executed  : " << trades.size() << std::endl;
    std::cout << "Execution Time         : " << execution_time << " seconds" << std::endl;
    std::cout << "Orders per Second      : " << (NUM_ORDERS / execution_time) << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
