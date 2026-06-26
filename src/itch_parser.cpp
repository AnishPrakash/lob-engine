#include "itch_parser.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <bit> 

static inline uint16_t ntoh16(uint16_t v) { return __builtin_bswap16(v); }
static inline uint32_t ntoh32(uint32_t v) { return __builtin_bswap32(v); }
static inline uint64_t ntoh64(uint64_t v) { return __builtin_bswap64(v); }

ITCHParser::ITCHParser(MatchingEngine& e) : engine_(e) {}
uint64_t ITCHParser::decode_timestamp(const uint8_t* hi_ptr, const uint8_t* lo_ptr) {
    uint16_t hi; 
    std::memcpy(&hi, hi_ptr, 2); 
    hi = ntoh16(hi);
    
    uint32_t lo; 
    std::memcpy(&lo, lo_ptr, 4); 
    lo = ntoh32(lo);
    
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

void ITCHParser::handle_add(const uint8_t* buf) {
    ITCHAddOrder msg;
    std::memcpy(&msg, buf, sizeof(msg));

    Order o;
    o.order_id = ntoh64(msg.order_ref);
    o.timestamp = decode_timestamp(msg.timestamp_hi, reinterpret_cast<const uint8_t*>(&msg.timestamp_lo));
    o.price = static_cast<int64_t>(ntoh32(msg.price));
    o.qty = ntoh32(msg.shares);
    o.orig_qty = o.qty;
    o.side = (msg.buy_sell == 'B') ? Side::Buy : Side::Sell;
    o.type = OrdType::Limit;
    o.status = OrdStatus::New;
    std::memcpy(o.symbol, msg.stock, 8);

    engine_.add_order(o);
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

        switch (static_cast<ITCHMsgType>(msg_buf[0])) {
            case ITCHMsgType::AddOrder: handle_add(msg_buf); break;
            case ITCHMsgType::DeleteOrder: handle_delete(msg_buf); break;
            case ITCHMsgType::ExecuteOrder: handle_execute(msg_buf); break;
            default: break;
        }
        ++count;
    }
    return count;
}