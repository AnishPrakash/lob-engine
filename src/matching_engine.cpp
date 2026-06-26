#include "matching_engine.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <immintrin.h>

// Constructor accepts metrics pointer for the sidecar
MatchingEngine::MatchingEngine(FillCallback cb, Metrics* m) 
    : on_fill_(std::move(cb)), metrics_(m) {}

double MatchingEngine::calculate_vwap_simd(const HalfBook& side) {
    alignas(32) double prices[4] = {0};
    alignas(32) double qtys[4] = {0};

    uint32_t pi = side.best_ask_idx(); 
    for (int i = 0; i < 4 && pi != NULL_IDX; ++i) {
        prices[i] = static_cast<double>(pi) / 100.0;
        qtys[i] = static_cast<double>(side.levels[pi].total_qty);
        
        uint32_t next_pi = NULL_IDX;
        for (uint32_t p = pi + 1; p < MAX_PRICE_LEVELS; ++p) {
            if (side.is_active(p)) { next_pi = p; break; }
        }
        pi = next_pi;
    }

    __m256d v_prices = _mm256_load_pd(prices);
    __m256d v_qtys = _mm256_load_pd(qtys);
    __m256d v_products = _mm256_mul_pd(v_prices, v_qtys);
    
    double res[4];
    _mm256_store_pd(res, v_products);
    
    double total_pv = res[0] + res[1] + res[2] + res[3];
    double total_q = qtys[0] + qtys[1] + qtys[2] + qtys[3];
    
    return (total_q > 0) ? (total_pv / total_q) : 0.0;
}

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void MatchingEngine::insert_to_book(uint32_t idx, HalfBook& side) {
    Order& o = pool_.get(idx);
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

        // Metrics Sidecar Update (Atomic, non-blocking)
        if (fills % 1000 == 0 && metrics_) {
            metrics_->update(
                bids_.best_bid_idx() / 100.0, 
                asks_.best_ask_idx() / 100.0, 
                pool_.in_use(), 
                0.0 
            );
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