#include "order_book.hpp"
#include <cstring>

HalfBook::HalfBook() {
    // 1. Allocate the memory that was previously a nullptr
    levels = new PriceLevel[MAX_PRICE_LEVELS];
    std::memset(bitset, 0, sizeof(bitset));
}

HalfBook::~HalfBook() {
    delete[] levels;
}

void HalfBook::set_active(uint32_t price_idx) {
    bitset[price_idx >> 6] |= (1ULL << (price_idx & 63));
}

void HalfBook::clear_active(uint32_t price_idx) {
    bitset[price_idx >> 6] &= ~(1ULL << (price_idx & 63));
}

bool HalfBook::is_active(uint32_t price_idx) const {
    return (bitset[price_idx >> 6] & (1ULL << (price_idx & 63))) != 0;
}

uint32_t HalfBook::best_ask_idx() const {
    // Search upwards from price 0
    for (size_t i = 0; i < (MAX_PRICE_LEVELS / 64 + 1); ++i) {
        if (bitset[i] > 0) {
            uint64_t b = bitset[i];
            uint32_t bit = __builtin_ctzll(b); // Find first set bit (fast)
            return (i << 6) + bit;
        }
    }
    return NULL_IDX;
}

uint32_t HalfBook::best_bid_idx() const {
    // Search downwards from max price
    for (int i = (MAX_PRICE_LEVELS / 64); i >= 0; --i) {
        if (bitset[i] > 0) {
            uint64_t b = bitset[i];
            uint32_t bit = 63 - __builtin_clzll(b); // Find last set bit (fast)
            return (i << 6) + bit;
        }
    }
    return NULL_IDX;
}