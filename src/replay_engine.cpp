#include "replay_engine.hpp"
#include "itch_parser.hpp"
#include <thread>
#include <fstream>

ReplayEngine::ReplayEngine(MatchingEngine& e, double speed) 
    : engine_(e), speed_(speed), count_(0) {}

void ReplayEngine::start(const std::string& path) {
    running_.store(true);
    
    // We need a helper to increment the counter every time a message is parsed
    struct StatefulParser : public ITCHParser {
        StatefulParser(MatchingEngine& e, std::atomic<uint64_t>& c) 
            : ITCHParser(e), count(c) {}
        
        std::atomic<uint64_t>& count;
        
        // Override the call to increment the counter
        void handle_add(const uint8_t* buf) {
            ITCHParser::handle_add(buf);
            count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    StatefulParser parser(engine_, count_);
    parser.parse_file(path);
}