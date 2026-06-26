#pragma once
#include "itch_parser.hpp"
#include <string>
#include <chrono>
#include <atomic>

class ReplayEngine {
public:
    ReplayEngine(MatchingEngine& engine, double speed_multiplier = 1.0);

    
    void start(const std::string& itch_path);

    
    void stop() { running_.store(false); }

    uint64_t messages_processed() const { return count_.load(); }

private:
    MatchingEngine& engine_;
    double speed_;
    std::atomic<bool> running_{true};
    std::atomic<uint64_t> count_{0};
};