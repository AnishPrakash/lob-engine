#pragma once
#include "matching_engine.hpp"
#include <atomic>
#include <string>

class ReplayEngine {
public:
    ReplayEngine(MatchingEngine& e, double speed);
    void start(const std::string& path);
    void stop() { running_.store(false); }
    uint64_t messages_processed() const { return count_.load(); }

private:
    MatchingEngine& engine_;
    double speed_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> count_{0};
};