#pragma once
#include "order.hpp"
#include <cstdint>
#include <array>

static constexpr uint32_t MAX_PRICE_LEVELS = 10000000;
static constexpr uint32_t NULL_IDX = UINT32_MAX;

struct PriceLevel {
    uint32_t head = NULL_IDX;
    uint32_t tail = NULL_IDX;
    uint32_t count = 0;
    uint64_t total_qty = 0;
};

struct HalfBook {
    PriceLevel* levels = nullptr;
    uint64_t bitset[MAX_PRICE_LEVELS / 64 + 1] = {0};

    HalfBook();
    ~HalfBook();

    void set_active(uint32_t price_idx);
    void clear_active(uint32_t price_idx);
    bool is_active(uint32_t price_idx) const;
    
    uint32_t best_bid_idx() const; 
    uint32_t best_ask_idx() const; 
};