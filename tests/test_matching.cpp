#include "matching_engine.hpp"
#include <cassert>
#include <iostream>
#include <vector>

static MatchingEngine make_engine(std::vector<Fill>& fills) {
    return MatchingEngine([&fills](const Fill& f) {
        fills.push_back(f);
    }, nullptr);
}

void test_simple_cross() {
    std::vector<Fill> fills;
    auto eng = make_engine(fills);
    
    Order sell;
    sell.order_id = 1; sell.price = 1000'0000; sell.qty = 100;
    sell.orig_qty = 100; sell.side = Side::Sell;
    sell.type = OrdType::Limit; sell.status = OrdStatus::New;
    sell.timestamp = 1;
    eng.add_order(sell);
    
    Order buy;
    buy.order_id = 2; buy.price = 1000'0000; buy.qty = 100;
    buy.orig_qty = 100; buy.side = Side::Buy;
    buy.type = OrdType::Limit; buy.status = OrdStatus::New;
    buy.timestamp = 2;
    eng.add_order(buy);
    
    assert(fills.size() == 1);
    assert(fills[0].qty == 100);
    std::cout << "PASS: test_simple_cross\n";
}

void test_partial_fill() {
    std::vector<Fill> fills;
    auto eng = make_engine(fills);
    
    Order sell;
    sell.order_id = 10; sell.price = 500'0000; sell.qty = 50;
    sell.orig_qty = 50; sell.side = Side::Sell;
    sell.type = OrdType::Limit; sell.status = OrdStatus::New;
    sell.timestamp = 1;
    eng.add_order(sell);
    
    Order buy;
    buy.order_id = 11; buy.price = 500'0000; buy.qty = 200;
    buy.orig_qty = 200; buy.side = Side::Buy;
    buy.type = OrdType::Limit; buy.status = OrdStatus::New;
    buy.timestamp = 2;
    eng.add_order(buy);
    
    assert(fills.size() == 1);
    assert(fills[0].qty == 50);
    std::cout << "PASS: test_partial_fill\n";
}

void test_self_match_prevention() {
    std::vector<Fill> fills;
    auto eng = make_engine(fills);
    
    Order sell;
    sell.order_id = 20; sell.price = 1000'0000; sell.qty = 100;
    sell.orig_qty = 100; sell.side = Side::Sell;
    sell.type = OrdType::Limit; sell.status = OrdStatus::New;
    sell.timestamp = 1; sell.trader_id = 42;
    eng.add_order(sell);
    
    Order buy;
    buy.order_id = 21; buy.price = 1000'0000; buy.qty = 100;
    buy.orig_qty = 100; buy.side = Side::Buy;
    buy.type = OrdType::Limit; buy.status = OrdStatus::New;
    buy.timestamp = 2; buy.trader_id = 42; // same trader!
    eng.add_order(buy);
    
    assert(fills.empty()); // no fill should occur
    std::cout << "PASS: test_self_match_prevention\n";
}

int main() {
    test_simple_cross();
    test_partial_fill();
    test_self_match_prevention();
    std::cout << "All tests passed!\n";
    return 0;
}