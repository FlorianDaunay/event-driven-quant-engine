#include "portfolio/Portfolio.hpp"
#include <cmath>

namespace quant
{

    Portfolio::Portfolio(EventQueue &event_queue, const HistoricalDataFeed &data_feed, double initial_cash)
        : event_queue_(event_queue), data_feed_(data_feed), initial_cash_(initial_cash), current_cash_(initial_cash), total_realized_pnl_(0.0) {}

    void Portfolio::onSignalEvent(const SignalEvent &signal)
    {
        OrderSide side = (signal.getSignalType() == SignalType::Buy) ? OrderSide::Buy : OrderSide::Sell;
        double quantity = signal.getStrength();

        if (quantity <= 0.0)
        {
            return;
        }

        auto order_event = std::make_shared<OrderEvent>(
            signal.getTimestamp(),
            signal.getSymbol(),
            OrderType::Market,
            side,
            quantity,
            0.0);

        event_queue_.push(order_event);
    }

    void Portfolio::onFillEvent(const FillEvent &fill)
    {
        const Symbol &symbol = fill.getSymbol();
        Position &pos = positions_[symbol];

        double fill_qty = (fill.getSide() == OrderSide::Buy) ? fill.getQuantity() : -fill.getQuantity();
        double fill_price = fill.getPrice();
        double commission = fill.getCommission();

        current_cash_ -= (fill_qty * fill_price) + commission;

        if (pos.quantity == 0.0)
        {
            pos.quantity = fill_qty;
            pos.average_price = fill_price;
        }
        else if ((pos.quantity > 0.0 && fill_qty > 0.0) || (pos.quantity < 0.0 && fill_qty < 0.0))
        {
            double total_qty = pos.quantity + fill_qty;
            pos.average_price = (pos.quantity * pos.average_price + fill_qty * fill_price) / total_qty;
            pos.quantity = total_qty;
        }
        else
        {
            double closed_qty = (std::abs(pos.quantity) < std::abs(fill_qty)) ? pos.quantity : -fill_qty;
            double trade_pnl = closed_qty * (fill_price - pos.average_price);

            pos.realized_pnl += trade_pnl;
            total_realized_pnl_ += trade_pnl;

            double remaining_qty = pos.quantity + fill_qty;
            if (remaining_qty == 0.0)
            {
                pos.quantity = 0.0;
                pos.average_price = 0.0;
            }
            else if ((pos.quantity > 0.0 && remaining_qty < 0.0) || (pos.quantity < 0.0 && remaining_qty > 0.0))
            {
                pos.quantity = remaining_qty;
                pos.average_price = fill_price;
            }
            else
            {
                pos.quantity = remaining_qty;
            }
        }

        updateMarketValue(symbol, fill_price);
    }

    void Portfolio::onMarketEvent(const MarketEvent &market)
    {
        updateMarketValue(market.getSymbol(), market.getBar().close);
    }

    void Portfolio::updateMarketValue(const Symbol &symbol, double current_price)
    {
        auto it = positions_.find(symbol);
        if (it != positions_.end())
        {
            Position &pos = it->second;
            pos.current_price = current_price;
            pos.unrealized_pnl = pos.quantity * (current_price - pos.average_price);
        }
    }

    double Portfolio::getInitialCash() const { return initial_cash_; }
    double Portfolio::getCash() const { return current_cash_; }

    double Portfolio::getTotalEquity() const
    {
        double equity = current_cash_;
        for (const auto &[symbol, pos] : positions_)
        {
            equity += pos.quantity * pos.current_price;
        }
        return equity;
    }

    double Portfolio::getRealizedPnL() const { return total_realized_pnl_; }

    double Portfolio::getUnrealizedPnL() const
    {
        double total_unrealized = 0.0;
        for (const auto &[symbol, pos] : positions_)
        {
            total_unrealized += pos.unrealized_pnl;
        }
        return total_unrealized;
    }

    const Position *Portfolio::getPosition(const Symbol &symbol) const
    {
        auto it = positions_.find(symbol);
        if (it != positions_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const std::map<Symbol, Position> &Portfolio::getAllPositions() const
    {
        return positions_;
    }

}