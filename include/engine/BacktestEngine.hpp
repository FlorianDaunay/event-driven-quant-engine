#ifndef BACKTESTENGINE_HPP
#define BACKTESTENGINE_HPP

#include <memory>
#include <string>
#include <cstdint>
#include "core/EventQueue.hpp"
#include "data/HistoricalDataFeed.hpp"
#include "execution/ExecutionEngine.hpp"
#include "portfolio/Portfolio.hpp"
#include "strategy/Strategy.hpp"

namespace quant
{

    struct BacktestResults
    {
        double initial_cash{0.0};
        double final_equity{0.0};
        double total_return_pct{0.0};
        double total_realized_pnl{0.0};
        double total_unrealized_pnl{0.0};
        size_t total_events_processed{0};
        uint64_t start_timestamp{0};
        uint64_t end_timestamp{0};
    };

    class BacktestEngine
    {
    public:
        BacktestEngine(double initial_cash = 100000.0, double commission_per_share = 0.005, double slippage_pct = 0.0001);
        ~BacktestEngine() = default;

        BacktestEngine(const BacktestEngine &) = delete;
        BacktestEngine &operator=(const BacktestEngine &) = delete;

        bool loadData(const Symbol &symbol, const std::string &filepath);
        bool loadData(const std::vector<std::pair<Symbol, std::string>> &datasets);
        void setStrategy(std::unique_ptr<Strategy> strategy);

        // Configuration des bornes temporelles
        void setStartDate(uint64_t timestamp) { start_timestamp_filter_ = timestamp; }
        void setEndDate(uint64_t timestamp) { end_timestamp_filter_ = timestamp; }

        BacktestResults run();

        const EventQueue &getEventQueue() const;
        const HistoricalDataFeed &getDataFeed() const;
        const ExecutionEngine &getExecutionEngine() const;
        const Portfolio &getPortfolio() const;

    private:
        EventQueue event_queue_;
        HistoricalDataFeed data_feed_;
        ExecutionEngine execution_engine_;
        Portfolio portfolio_;
        std::unique_ptr<Strategy> strategy_;

        size_t processed_events_count_{0};
        uint64_t start_timestamp_{0};
        uint64_t end_timestamp_{0};

        uint64_t start_timestamp_filter_{0};
        uint64_t end_timestamp_filter_{0};

        void processEvent(const std::shared_ptr<Event> &event);
    };

}

#endif