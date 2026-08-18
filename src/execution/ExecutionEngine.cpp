#include "execution/ExecutionEngine.hpp"
#include <cmath>
#include <algorithm>

namespace quant
{

    ExecutionEngine::ExecutionEngine(EventQueue &event_queue, const HistoricalDataFeed &data_feed, double commission_per_share, double slippage_pct)
        : event_queue_(event_queue), data_feed_(data_feed), commission_per_share_(commission_per_share), slippage_pct_(slippage_pct) {}

    void ExecutionEngine::processOrder(const OrderEvent &order)
    {
        double fill_price = calculateExecutionPrice(order.getSymbol(), order.getSide(), order.getOrderType(), order.getLimitPrice());

        if (fill_price <= 0.0)
        {
            return;
        }

        double commission = calculateCommission(order.getQuantity());

        auto fill_event = std::make_shared<FillEvent>(
            order.getTimestamp(),
            order.getSymbol(),
            order.getSide(),
            order.getQuantity(),
            fill_price,
            commission);

        event_queue_.push(fill_event);
    }

    void ExecutionEngine::setCommissionPerShare(double commission)
    {
        commission_per_share_ = commission;
    }

    void ExecutionEngine::setSlippagePct(double slippage_pct)
    {
        slippage_pct_ = slippage_pct;
    }

    double ExecutionEngine::calculateExecutionPrice(const Symbol &symbol, OrderSide side, OrderType order_type, double limit_price) const
    {
        const TradeBar *bar = data_feed_.getLatestBar(symbol);
        if (!bar)
        {
            return 0.0;
        }

        double base_price = bar->close;

        if (order_type == OrderType::Limit)
        {
            if (side == OrderSide::Buy && base_price > limit_price)
            {
                return 0.0;
            }
            if (side == OrderSide::Sell && base_price < limit_price)
            {
                return 0.0;
            }
            base_price = limit_price;
        }

        double slippage = base_price * slippage_pct_;
        if (side == OrderSide::Buy)
        {
            return base_price + slippage;
        }
        else
        {
            return std::max(0.0, base_price - slippage);
        }
    }

    double ExecutionEngine::calculateCommission(double quantity) const
    {
        return std::abs(quantity) * commission_per_share_;
    }

}