#include <benchmark/benchmark.h>
#include "matching_engine.hpp"
#include <random>

// --- Benchmark: add limit orders at random prices ---
static void BM_AddLimitOrder(benchmark::State& state) {
    MatchingEngine eng([](const Fill&) {});
    std::mt19937 rng(42);
    std::uniform_int_distribution<int64_t> price_dist(100'0000, 200'0000);
    std::uniform_int_distribution<uint32_t> qty_dist(1, 1000);
    
    uint64_t oid = 1;
    for (auto _ : state) {
        Order o;
        o.order_id = oid++;
        o.price = price_dist(rng);
        o.qty = qty_dist(rng);
        o.orig_qty = o.qty;
        o.side = (oid % 2 == 0) ? Side::Buy : Side::Sell;
        o.type = OrdType::Limit;
        o.status = OrdStatus::New;
        o.timestamp = oid * 1000;
        
        benchmark::DoNotOptimize(eng.add_order(o));
    }
}
BENCHMARK(BM_AddLimitOrder)->Iterations(1'000'000);

// --- Benchmark: market order that walks the book ---
static void BM_MarketOrder(benchmark::State& state) {
    MatchingEngine eng([](const Fill&) {});
    
    // Pre-populate 10,000 resting sell orders
    for (uint64_t i = 1; i <= 10000; ++i) {
        Order o;
        o.order_id = i;
        o.price = 1500'0000 + static_cast<int64_t>(i * 10);
        o.qty = 100;
        o.orig_qty = 100;
        o.side = Side::Sell;
        o.type = OrdType::Limit;
        o.status = OrdStatus::New;
        o.timestamp = i;
        eng.add_order(o);
    }
    
    uint64_t oid = 100001;
    for (auto _ : state) {
        Order o;
        o.order_id = oid++;
        o.price = 0; // market
        o.qty = 10;
        o.orig_qty = 10;
        o.side = Side::Buy;
        o.type = OrdType::Market;
        o.status = OrdStatus::New;
        o.timestamp = oid * 1000;
        
        benchmark::DoNotOptimize(eng.add_order(o));
    }
}
BENCHMARK(BM_MarketOrder)->Iterations(100'000);

BENCHMARK_MAIN();