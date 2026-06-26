#pragma once
#include "itch_parser.hpp"
#include <string>
#include <chrono>
#include <atomic>

class ReplayEngine {
public:
    ReplayEngine(MatchingEngine& engine, double speed_multiplier = 1.0);

    // Start replay; blocks until complete or stop() called.
    void start(const std::string& itch_path);

    // Signal replay to stop (call from another thread).
    void stop() { running_.store(false); }

    uint64_t messages_processed() const { return count_.load(); }

private:
    MatchingEngine& engine_;
    double speed_;
    std::atomic<bool> running_{true};
    std::atomic<uint64_t> count_{0};
};