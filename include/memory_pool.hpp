#pragma once
#include "order.hpp"
#include <vector>
#include <stack>
#include <stdexcept>

// 15 million orders
static constexpr uint32_t POOL_SIZE = 15'000'000;

class OrderPool {
public:
    OrderPool() {
        // One-time allocation: 15M orders * 64 bytes = ~960 MB
        pool_.resize(POOL_SIZE);
        
        // Pre-fill free list in reverse so index 0 is acquired first
        for (uint32_t i = POOL_SIZE; i > 0; --i) {
            free_list_.push(i - 1);
        }
    }

    uint32_t acquire() {
        if (free_list_.empty()) {
            throw std::runtime_error("OrderPool exhausted");
        }
        uint32_t idx = free_list_.top();
        free_list_.pop();
        return idx;
    }

    void release(uint32_t idx) {
        pool_[idx] = Order(); // zero-init for reuse
        free_list_.push(idx);
    }

    Order& get(uint32_t idx) { return pool_[idx]; }
    const Order& get(uint32_t idx) const { return pool_[idx]; }

    uint32_t capacity() const { return POOL_SIZE; }
    
    uint32_t in_use() const {
        return POOL_SIZE - static_cast<uint32_t>(free_list_.size());
    }

private:
    std::vector<Order> pool_;
    std::stack<uint32_t> free_list_;
};