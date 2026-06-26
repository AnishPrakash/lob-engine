#include <grpcpp/grpcpp.h>
#include "metrics.grpc.pb.h" // Generated from your proto
#include "metrics_manager.hpp"
#include "metrics.pb.h"
class MetricsServiceImpl final : public lob::metrics::MetricsService::Service {
public:
    MetricsServiceImpl(Metrics& m) : metrics_(m) {}

    grpc::Status GetMarketStats(grpc::ServerContext* context, 
                               const lob::metrics::EmptyRequest* request, 
                               lob::metrics::MarketStats* reply) override {
        
        // Non-blocking read from atomics
        reply->set_bid_vwap(metrics_.bid_vwap.load(std::memory_order_relaxed));
        reply->set_ask_vwap(metrics_.ask_vwap.load(std::memory_order_relaxed));
        reply->set_active_orders(metrics_.active_orders.load(std::memory_order_relaxed));
        reply->set_p99_latency_ms(metrics_.p99_latency_ms.load(std::memory_order_relaxed));
        
        return grpc::Status::OK;
    }

private:
    Metrics& metrics_;
};