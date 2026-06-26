#include "fix_output.hpp"
#include <fmt/format.h>
#include <numeric>

static constexpr char SOH = '\x01';

std::string fill_to_fix42(const Fill& f, uint64_t exec_id) {
    double avg_px = static_cast<double>(f.price) / 10000.0;
    
    std::string body = fmt::format(
        "35=8{0}17={1}{0}39=2{0}14={2}{0}6={3:.4f}{0}55=FILL{0}60={4}{0}",
        SOH, exec_id, f.qty, avg_px, f.timestamp
    );

    uint32_t cksum = 0;
    for (unsigned char c : body) {
        cksum += c;
    }
    
    std::string header = fmt::format("8=FIX.4.2{0}9={1}{0}", SOH, body.size());
    for (unsigned char c : header) {
        cksum += c;
    }
    cksum %= 256;

    return fmt::format("{0}{1}{2}10={3:03d}{0}", SOH, header, body, cksum);
}