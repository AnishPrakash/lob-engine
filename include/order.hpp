#pragma once
#include <cstdint>
#include <cstring>

// Order side
enum class Side : uint8_t { Buy = 0, Sell = 1 };

// Order type
enum class OrdType : uint8_t { Limit = 0, Market = 1, IOC = 2, FOK = 3 };

// Order status
enum class OrdStatus : uint8_t { New = 0, PartFill = 1, Filled = 2, Cancelled = 3 };

// Plain-Old-Data Order struct (fits perfectly in a 64-byte cache line)
struct alignas(64) Order {
    uint64_t order_id = 0;
    uint64_t timestamp = 0;      // nanoseconds
    int64_t price = 0;           // fixed-point: actual price * 10000
    uint32_t qty = 0;            // remaining quantity
    uint32_t orig_qty = 0;       // original quantity
    uint32_t trader_id = 0;      // for self-match prevention
    uint32_t next_idx = UINT32_MAX; // Intrusive list link
    uint32_t prev_idx = UINT32_MAX;
    char symbol[8] = {0};        // fixed-width, no heap allocation
    Side side = Side::Buy;
    OrdType type = OrdType::Limit;
    OrdStatus status = OrdStatus::New;
    uint8_t _pad[5] = {0};       // explicit padding to reach exactly 64 bytes

    // Helpers
    bool is_buy() const { return side == Side::Buy; }
    bool is_sell() const { return side == Side::Sell; }
    bool is_active() const {
        return status == OrdStatus::New || status == OrdStatus::PartFill;
    }
    void set_symbol(const char* s) {
        std::strncpy(symbol, s, 8);
    }
};

static_assert(sizeof(Order) == 64, "Order must be exactly 64 bytes");