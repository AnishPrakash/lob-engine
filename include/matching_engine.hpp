#pragma once
#include "order.hpp"
#include "order_book.hpp"
#include "memory_pool.hpp"
#include <functional>
#include <unordered_map>
// Add this near the top of include/matching_engine.hpp
#include <pthread.h> // Ensure this header is included

void pin_to_core(int core_id);

struct Fill {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    int64_t price;
    uint32_t qty;
    uint64_t timestamp;
};

using FillCallback = std::function<void(const Fill&)>;

class MatchingEngine {
public:
    explicit MatchingEngine(FillCallback cb);

    int add_order(Order& o);
    int cancel_order(uint64_t order_id);
    int modify_order(uint64_t order_id, uint32_t new_qty);

    const HalfBook& bids() const { return bids_; }
    const HalfBook& asks() const { return asks_; }
    const OrderPool& pool() const { return pool_; }
    uint32_t active_orders() const { return pool_.in_use(); }

private:
    HalfBook bids_;
    HalfBook asks_;
    OrderPool pool_; 
    FillCallback on_fill_;

   
    std::unordered_map<uint64_t, uint32_t> id_to_idx_;

    void insert_to_book(uint32_t idx, HalfBook& side);
    void remove_from_book(uint32_t idx, HalfBook& side);
    int match_order(Order& incoming, HalfBook& passive_side, bool is_bid_aggressor);
};