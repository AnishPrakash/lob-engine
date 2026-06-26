#include "itch_parser.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

static inline uint16_t ntoh16(uint16_t v) { return __builtin_bswap16(v); }
static inline uint32_t ntoh32(uint32_t v) { return __builtin_bswap32(v); }
static inline uint64_t ntoh64(uint64_t v) { return __builtin_bswap64(v); }

ITCHParser::ITCHParser(MatchingEngine& e) : engine_(e) {}

void ITCHParser::handle_add(const uint8_t* buf) {
    Order o;
    
    uint64_t order_ref;
    std::memcpy(&order_ref, buf + 11, 8);
    o.order_id = ntoh64(order_ref);

    uint16_t ts_hi;
    std::memcpy(&ts_hi, buf + 5, 2);
    uint32_t ts_lo;
    std::memcpy(&ts_lo, buf + 7, 4);
    o.timestamp = (static_cast<uint64_t>(ntoh16(ts_hi)) << 32) | ntoh32(ts_lo);

    o.side = (buf[19] == 'B') ? Side::Buy : Side::Sell;

    uint32_t shares;
    std::memcpy(&shares, buf + 20, 4);
    o.qty = ntoh32(shares);
    o.orig_qty = o.qty;

    std::memcpy(o.symbol, buf + 24, 8);

    uint32_t price;
    std::memcpy(&price, buf + 32, 4);
    o.price = static_cast<int64_t>(ntoh32(price)) / 100;

    o.type = OrdType::Limit;
    o.status = OrdStatus::New;

    engine_.add_order(o);
    static std::ofstream log("debug.log", std::ios::app);
    log << "Added order: " << o.order_id << " Price: " << o.price << std::endl;
}

void ITCHParser::handle_delete(const uint8_t* buf) {}
void ITCHParser::handle_replace(const uint8_t* buf) {}
void ITCHParser::handle_execute(const uint8_t* buf) {}

uint64_t ITCHParser::parse_file(std::string_view path) {
    std::ifstream f(path.data(), std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open ITCH file");

    uint64_t count = 0;
    uint8_t len_buf[2];
    uint8_t msg_buf[128];

    while (f.read(reinterpret_cast<char*>(len_buf), 2)) {
        uint16_t len = ntoh16(*reinterpret_cast<uint16_t*>(len_buf));
        if (len == 0 || len > 128) continue;

        f.read(reinterpret_cast<char*>(msg_buf), len);
        if (!f) break;

        switch (msg_buf[0]) {
            case 'A': handle_add(msg_buf); break;
            default: break; 
        }
        ++count;
    }
    return count;
}