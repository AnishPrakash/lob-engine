#pragma once
#include <atomic>

struct Metrics {
    std::atomic<double> bid_vwap{0.0};
    std::atomic<double> ask_vwap{0.0};
    std::atomic<uint64_t> active_orders{0};
    std::atomic<double> p99_latency_ms{0.0};

    // Called by the Hot Path (Matching Engine)
    void update(double bid, double ask, uint64_t orders, double lat) {
        bid_vwap.store(bid, std::memory_order_relaxed);
        ask_vwap.store(ask, std::memory_order_relaxed);
        active_orders.store(orders, std::memory_order_relaxed);
        p99_latency_ms.store(lat, std::memory_order_relaxed);
    }
};