#ifndef PORTFOLIO_HPP
#define PORTFOLIO_HPP

#include <map>
#include <memory>
#include "core/Types.hpp"
#include "core/Event.hpp"
#include "core/EventQueue.hpp"
#include "data/HistoricalDataFeed.hpp"

namespace quant
{

    struct Position
    {
        double quantity{0.0};
        double average_price{0.0};
        double current_price{0.0};
        double unrealized_pnl{0.0};
        double realized_pnl{0.0};
    };

    class Portfolio
    {
    public:
        Portfolio(EventQueue &event_queue, const HistoricalDataFeed &data_feed, double initial_cash = 100000.0);
        ~Portfolio() = default;

        Portfolio(const Portfolio &) = delete;
        Portfolio &operator=(const Portfolio &) = delete;

        void onSignalEvent(const SignalEvent &signal);
        void onFillEvent(const FillEvent &fill);
        void onMarketEvent(const MarketEvent &market);

        double getInitialCash() const;
        double getCash() const;
        double getTotalEquity() const;
        double getRealizedPnL() const;
        double getUnrealizedPnL() const;
        void addCash(double amount)
        {
            current_cash_ += amount;
        }

        const Position *getPosition(const Symbol &symbol) const;
        const std::map<Symbol, Position> &getAllPositions() const;

    private:
        EventQueue &event_queue_;
        const HistoricalDataFeed &data_feed_;

        double initial_cash_;
        double current_cash_;
        double total_realized_pnl_;

        std::map<Symbol, Position> positions_;

        void updateMarketValue(const Symbol &symbol, double current_price);
    };

}

#endif