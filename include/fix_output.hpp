#pragma once
#include "matching_engine.hpp"
#include <string>

// Serialise a Fill into a FIX 4.2 Execution Report string.
// Tags used: 8 (BeginString), 35 (MsgType=8), 17 (ExecID),
// 39 (OrdStatus=2 Filled), 14 (CumQty), 6 (AvgPx),
// 60 (TransactTime), 10 (Checksum).
std::string fill_to_fix42(const Fill& f, uint64_t exec_id);