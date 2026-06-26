#include "matching_engine.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <pthread.h>

MatchingEngine::MatchingEngine(FillCallback cb) : on_fill_(std::move(cb)) {}
void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void MatchingEngine::insert_to_book(uint32_t idx, HalfBook& side) {
    Order& o = pool_.get(idx);
    static std::ofstream debug("price_debug.log", std::ios::app);
    debug << "Inserting Price: " << o.price << " | PI: " << (uint32_t)o.price << std::endl;
    uint32_t pi = static_cast<uint32_t>(o.price);
    PriceLevel& lvl = side.levels[pi];

    if (lvl.head == NULL_IDX) {
        lvl.head = lvl.tail = idx;
        o.next_idx = o.prev_idx = NULL_IDX;
        side.set_active(pi);
    } else {
        pool_.get(lvl.tail).next_idx = idx;
        o.prev_idx = lvl.tail;
        o.next_idx = NULL_IDX;
        lvl.tail = idx;
    }
    ++lvl.count;
    lvl.total_qty += o.qty;
}

void MatchingEngine::remove_from_book(uint32_t idx, HalfBook& side) {
    Order& o = pool_.get(idx);
    uint32_t pi = static_cast<uint32_t>(o.price);
    PriceLevel& lvl = side.levels[pi];

    if (o.prev_idx != NULL_IDX) pool_.get(o.prev_idx).next_idx = o.next_idx;
    else lvl.head = o.next_idx;

    if (o.next_idx != NULL_IDX) pool_.get(o.next_idx).prev_idx = o.prev_idx;
    else lvl.tail = o.prev_idx;

    lvl.total_qty = (lvl.total_qty > o.qty) ? lvl.total_qty - o.qty : 0;
    if (--lvl.count == 0) side.clear_active(pi);
}

int MatchingEngine::match_order(Order& inc, HalfBook& passive, bool inc_is_bid) {
    int fills = 0;
    while (inc.qty > 0) {
        uint32_t best_pi = inc_is_bid ? passive.best_ask_idx() : passive.best_bid_idx();
        if (best_pi == NULL_IDX) break;
        if (inc.type == OrdType::Limit) {
            if (inc_is_bid && inc.price < static_cast<int64_t>(best_pi)) break;
            if (!inc_is_bid && inc.price > static_cast<int64_t>(best_pi)) break;
        }

        uint32_t pass_idx = passive.levels[best_pi].head;
        Order& pass = pool_.get(pass_idx);

        if (pass.trader_id != 0 && pass.trader_id == inc.trader_id) {
            remove_from_book(pass_idx, passive);
            pass.status = OrdStatus::Cancelled;
            pool_.release(pass_idx);
            id_to_idx_.erase(pass.order_id);
            continue;
        }

        uint32_t fill_qty = std::min(inc.qty, pass.qty);
        Fill f;
        f.buy_order_id = inc_is_bid ? inc.order_id : pass.order_id;
        f.sell_order_id = inc_is_bid ? pass.order_id : inc.order_id;
        f.price = pass.price;
        f.qty = fill_qty;
        f.timestamp = inc.timestamp;
        
        on_fill_(f);
        ++fills;

        inc.qty -= fill_qty;
        pass.qty -= fill_qty;
        passive.levels[best_pi].total_qty -= fill_qty;

        pass.status = (pass.qty == 0) ? OrdStatus::Filled : OrdStatus::PartFill;
        inc.status = (inc.qty == 0) ? OrdStatus::Filled : OrdStatus::PartFill;

        if (pass.qty == 0) {
            remove_from_book(pass_idx, passive);
            pool_.release(pass_idx);
            id_to_idx_.erase(pass.order_id);
        }
    }
    return fills;
}

int MatchingEngine::add_order(Order& o) {
    uint32_t idx = pool_.acquire();
    pool_.get(idx) = o;
    id_to_idx_[o.order_id] = idx;

    HalfBook& passive = o.is_buy() ? asks_ : bids_;
    int fills = match_order(pool_.get(idx), passive, o.is_buy());

    Order& stored = pool_.get(idx);
    if (stored.qty > 0 && stored.type != OrdType::Market && stored.type != OrdType::IOC && stored.type != OrdType::FOK) {
        HalfBook& own = o.is_buy() ? bids_ : asks_;
        insert_to_book(idx, own);
    } else if (stored.qty > 0) {
        // Cancel remainder for IOC/FOK/Market
        stored.status = OrdStatus::Cancelled;
        pool_.release(idx);
        id_to_idx_.erase(o.order_id);
    }
    return fills;
}

int MatchingEngine::cancel_order(uint64_t order_id) {
    auto it = id_to_idx_.find(order_id);
    if (it == id_to_idx_.end()) return 0;

    uint32_t idx = it->second;
    Order& o = pool_.get(idx);
    HalfBook& side = o.is_buy() ? bids_ : asks_;
    
    remove_from_book(idx, side);
    o.status = OrdStatus::Cancelled;
    pool_.release(idx);
    id_to_idx_.erase(it);
    return 1;
}