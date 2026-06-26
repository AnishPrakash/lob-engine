#include "replay_engine.hpp"
#include <thread>
#include <fstream>

ReplayEngine::ReplayEngine(MatchingEngine& e, double speed) 
    : engine_(e), speed_(speed) {}

void ReplayEngine::start(const std::string& path) {
    running_.store(true);
    ITCHParser parser(engine_);
    
    // For simplicity in this phase, replay at full speed.
    // Speed-controlled replay requires buffering timestamps.
    // TODO: implement wall-clock throttle using first_ts + elapsed/speed_
    
    uint64_t n = parser.parse_file(path);
    count_.store(n);
}