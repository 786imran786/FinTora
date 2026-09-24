#pragma once

#include "Protocol.hpp"
#include <atomic>
#include <chrono>
#include <mutex>

namespace fintora {

class Metrics {
public:
    Metrics();

    void recordOrder();
    void recordTrade(int64_t volume);
    void recordCancel();
    void setActiveOrders(int64_t count);

    MetricsData snapshot() const;

private:
    std::atomic<int64_t> ordersProcessed_{0};
    std::atomic<int64_t> tradesExecuted_{0};
    std::atomic<int64_t> activeOrders_{0};
    std::atomic<int64_t> totalVolume_{0};
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace fintora
