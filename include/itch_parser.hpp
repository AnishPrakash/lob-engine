#pragma once
#include "matching_engine.hpp"
#include <cstdint>
#include <string_view>

// ITCH 5.0 message type tags
enum class ITCHMsgType : char {
    AddOrder = 'A',
    DeleteOrder = 'D',
    ReplaceOrder = 'U',
    ExecuteOrder = 'E',
    CancelOrder = 'X',
    SystemEvent = 'S',
    StockDirectory = 'R'
};

// Raw ITCH AddOrder message layout (big-endian on wire)
#pragma pack(push, 1)
struct ITCHAddOrder {
    char msg_type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp_hi[2]; // 16-bit high part
    uint32_t timestamp_lo;   // 32-bit low part
    uint64_t order_ref;
    char buy_sell;           // 'B' or 'S'
    uint32_t shares;
    char stock[8];
    uint32_t price;          // price * 10000
};
#pragma pack(pop)

class ITCHParser {
public:
    explicit ITCHParser(MatchingEngine& engine);
    
    // Parse entire ITCH file, returns total messages processed.
    uint64_t parse_file(std::string_view path);

private:
    MatchingEngine& engine_;
    void handle_add(const uint8_t* buf);
    void handle_delete(const uint8_t* buf);
    void handle_replace(const uint8_t* buf);
    void handle_execute(const uint8_t* buf);
    
    static uint64_t decode_timestamp(const uint8_t* ts_hi_ptr, const uint8_t* ts_lo_ptr);
};