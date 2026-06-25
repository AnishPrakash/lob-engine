#include "order_book.hpp"
#include <bit>
#include <cstdlib>
#include <stdexcept>

HalfBook::HalfBook() {
    // Single large allocation at startup - never again on hot path.
    levels = static_cast<PriceLevel*>(std::calloc(MAX_PRICE_LEVELS, sizeof(PriceLevel)));
    if (!levels) throw std::runtime_error("OOM: HalfBook");
}

HalfBook::~HalfBook() { 
    std::free(levels); 
}

void HalfBook::set_active(uint32_t idx) {
    bitset[idx / 64] |= (1ULL << (idx % 64));
}

void HalfBook::clear_active(uint32_t idx) {
    bitset[idx / 64] &= ~(1ULL << (idx % 64));
}

bool HalfBook::is_active(uint32_t idx) const {
    return (bitset[idx / 64] >> (idx % 64)) & 1ULL;
}

// Best bid: highest active price index.
uint32_t HalfBook::best_bid_idx() const {
    // Scan from top word downward.
    int top = static_cast<int>(MAX_PRICE_LEVELS / 64);
    for (int w = top; w >= 0; --w) {
        if (bitset[w]) {
            int bit = 63 - __builtin_clzll(bitset[w]);
            return static_cast<uint32_t>(w * 64 + bit);
        }
    }
    return NULL_IDX;
}

// Best ask: lowest active price index.
uint32_t HalfBook::best_ask_idx() const {
    uint32_t words = MAX_PRICE_LEVELS / 64 + 1;
    for (uint32_t w = 0; w < words; ++w) {
        if (bitset[w]) {
            int bit = __builtin_ctzll(bitset[w]);
            return static_cast<uint32_t>(w * 64 + bit);
        }
    }
    return NULL_IDX;
}