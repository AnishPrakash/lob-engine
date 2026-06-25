#pragma once
#include "order.hpp"
#include <cstdint>
#include <array>

// Price is stored as Int64 fixed-point (price * 10000).
// MAX_PRICE_LEVELS covers prices 0 to 999.9999 in steps of 0.0001.
static constexpr uint32_t MAX_PRICE_LEVELS = 10000000;
static constexpr uint32_t NULL_IDX = UINT32_MAX;

// One price-level entry: head and tail of the intrusive order queue.
struct PriceLevel {
    uint32_t head = NULL_IDX;
    uint32_t tail = NULL_IDX;
    uint32_t count = 0;
    uint64_t total_qty = 0;
};

// Separate bid and ask sides (indexed by fixed-point price).
// Backed by flat arrays — no heap nodes, no pointer chasing.
struct HalfBook {
    // We use dynamic allocation once at startup only.
    PriceLevel* levels = nullptr;
    uint64_t bitset[MAX_PRICE_LEVELS / 64 + 1] = {0};

    HalfBook();
    ~HalfBook();

    void set_active(uint32_t price_idx);
    void clear_active(uint32_t price_idx);
    bool is_active(uint32_t price_idx) const;
    
    uint32_t best_bid_idx() const; // highest set bit
    uint32_t best_ask_idx() const; // lowest set bit
};