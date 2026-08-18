#ifndef EXECUTIONENGINE_HPP
#define EXECUTIONENGINE_HPP

#include <memory>
#include "core/Event.hpp"
#include "core/EventQueue.hpp"
#include "data/HistoricalDataFeed.hpp"

namespace quant
{

    class ExecutionEngine
    {
    public:
        ExecutionEngine(EventQueue &event_queue, const HistoricalDataFeed &data_feed, double commission_per_share = 0.005, double slippage_pct = 0.0001);
        ~ExecutionEngine() = default;

        ExecutionEngine(const ExecutionEngine &) = delete;
        ExecutionEngine &operator=(const ExecutionEngine &) = delete;

        void processOrder(const OrderEvent &order);

        void setCommissionPerShare(double commission);
        void setSlippagePct(double slippage_pct);

    private:
        EventQueue &event_queue_;
        const HistoricalDataFeed &data_feed_;
        double commission_per_share_;
        double slippage_pct_;

        double calculateExecutionPrice(const Symbol &symbol, OrderSide side, OrderType order_type, double limit_price) const;
        double calculateCommission(double quantity) const;
    };

}

#endif