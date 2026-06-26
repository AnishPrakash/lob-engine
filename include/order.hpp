#pragma once
#include <cstdint>

// This line MUST be here!
static constexpr uint32_t NULL_IDX = 0xFFFFFFFF;

enum class Side { Buy, Sell };
enum class OrdType { Limit, Market, IOC, FOK };
enum class OrdStatus { New, PartFill, Filled, Cancelled };

struct Order {
    uint64_t order_id = 0;
    uint64_t trader_id = 0; 
    uint64_t timestamp = 0;
    int64_t price = 0;
    uint32_t qty = 0;
    uint32_t orig_qty = 0;
    Side side = Side::Buy;
    OrdType type = OrdType::Limit;
    OrdStatus status = OrdStatus::New;
    char symbol[8] = {0};

    uint32_t prev_idx = NULL_IDX;
    uint32_t next_idx = NULL_IDX;

    bool is_buy() const { return side == Side::Buy; }
};