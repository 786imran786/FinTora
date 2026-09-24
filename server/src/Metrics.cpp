#include "Metrics.hpp"

namespace fintora {

Metrics::Metrics()
    : startTime_(std::chrono::steady_clock::now()) {}

void Metrics::recordOrder() {
    ordersProcessed_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::recordTrade(int64_t volume) {
    tradesExecuted_.fetch_add(1, std::memory_order_relaxed);
    totalVolume_.fetch_add(volume, std::memory_order_relaxed);
}

void Metrics::recordCancel() {
    // Cancels counted as processed orders
}

void Metrics::setActiveOrders(int64_t count) {
    activeOrders_.store(count, std::memory_order_relaxed);
}

MetricsData Metrics::snapshot() const {
    MetricsData data;
    data.ordersProcessed = ordersProcessed_.load(std::memory_order_relaxed);
    data.tradesExecuted = tradesExecuted_.load(std::memory_order_relaxed);
    data.activeOrders = activeOrders_.load(std::memory_order_relaxed);
    data.totalVolume = totalVolume_.load(std::memory_order_relaxed);

    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    double seconds = std::chrono::duration<double>(elapsed).count();
    data.ordersPerSecond = (seconds > 0.0) ? static_cast<double>(data.ordersProcessed) / seconds : 0.0;

    return data;
}

} // namespace fintora
