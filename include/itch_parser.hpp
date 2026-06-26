#pragma once
#include "matching_engine.hpp"
#include <string_view>
#include <cstdint>

enum class ITCHMsgType {
    AddOrder = 'A',
    DeleteOrder = 'D',
    ExecuteOrder = 'E',
    ReplaceOrder = 'U'
};

class ITCHParser {
public:
    ITCHParser(MatchingEngine& e);
    uint64_t parse_file(std::string_view path);

protected: // Changed from private to protected so StateFulParser can use/override
    void handle_add(const uint8_t* buf);
    void handle_delete(const uint8_t* buf);
    void handle_execute(const uint8_t* buf);
    void handle_replace(const uint8_t* buf);

private:
    MatchingEngine& engine_;
    uint64_t decode_timestamp(const uint8_t* hi_ptr, const uint8_t* lo_ptr);
};