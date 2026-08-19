#include "engine/BacktestEngine.hpp"
#include <iostream>

namespace quant
{

    BacktestEngine::BacktestEngine(double initial_cash, double commission_per_share, double slippage_pct)
        : data_feed_(event_queue_),
          execution_engine_(event_queue_, data_feed_, commission_per_share, slippage_pct),
          portfolio_(event_queue_, data_feed_, initial_cash) {}

    bool BacktestEngine::loadData(const Symbol &symbol, const std::string &filepath)
    {
        return data_feed_.loadCSV(symbol, filepath);
    }

    bool BacktestEngine::loadData(const std::vector<std::pair<Symbol, std::string>> &datasets)
    {
        return data_feed_.loadMultipleCSV(datasets);
    }

    void BacktestEngine::setStrategy(std::unique_ptr<Strategy> strategy)
    {
        strategy_ = std::move(strategy);
    }

    BacktestResults BacktestEngine::run()
    {
        if (!strategy_)
        {
            throw std::runtime_error("Engine Error: No strategy assigned prior to run()");
        }

        processed_events_count_ = 0;
        start_timestamp_ = 0;
        end_timestamp_ = 0;

        bool running = true;
        while (data_feed_.hasNext() && running)
        {
            data_feed_.step();

            while (!event_queue_.empty() && running)
            {
                auto event = event_queue_.pop();
                if (event)
                {
                    if (event->getType() == EventType::Market)
                    {
                        const auto &market_event = static_cast<const MarketEvent &>(*event);
                        uint64_t ts = market_event.getBar().timestamp;

                        // Arrêt strict si la date de fin est dépassée
                        if (end_timestamp_filter_ > 0 && ts > end_timestamp_filter_)
                        {
                            running = false;
                            break;
                        }

                        // Ignore les événements antérieurs à la date de début demandée (attend les données disponibles)
                        if (start_timestamp_filter_ > 0 && ts < start_timestamp_filter_)
                        {
                            continue;
                        }

                        if (start_timestamp_ == 0)
                        {
                            start_timestamp_ = ts;
                        }
                        end_timestamp_ = ts;
                    }

                    processEvent(event);
                    processed_events_count_++;
                }
            }
        }

        BacktestResults results;
        results.initial_cash = portfolio_.getInitialCash();
        results.final_equity = portfolio_.getTotalEquity();
        results.total_realized_pnl = portfolio_.getRealizedPnL();
        results.total_unrealized_pnl = portfolio_.getUnrealizedPnL();
        results.total_events_processed = processed_events_count_;
        results.start_timestamp = start_timestamp_;
        results.end_timestamp = end_timestamp_;

        if (results.initial_cash > 0.0)
        {
            results.total_return_pct = ((results.final_equity - results.initial_cash) / results.initial_cash) * 100.0;
        }
        else
        {
            results.total_return_pct = 0.0;
        }

        return results;
    }

    void BacktestEngine::processEvent(const std::shared_ptr<Event> &event)
    {
        switch (event->getType())
        {
        case EventType::Market:
        {
            const auto &market_event = static_cast<const MarketEvent &>(*event);
            portfolio_.onMarketEvent(market_event);
            strategy_->onMarketEvent(market_event);
            break;
        }
        case EventType::Signal:
        {
            const auto &signal_event = static_cast<const SignalEvent &>(*event);
            portfolio_.onSignalEvent(signal_event);
            break;
        }
        case EventType::Order:
        {
            const auto &order_event = static_cast<const OrderEvent &>(*event);
            execution_engine_.processOrder(order_event);
            break;
        }
        case EventType::Fill:
        {
            const auto &fill_event = static_cast<const FillEvent &>(*event);
            portfolio_.onFillEvent(fill_event);
            break;
        }
        }
    }

    const EventQueue &BacktestEngine::getEventQueue() const { return event_queue_; }
    const HistoricalDataFeed &BacktestEngine::getDataFeed() const { return data_feed_; }
    const ExecutionEngine &BacktestEngine::getExecutionEngine() const { return execution_engine_; }
    const Portfolio &BacktestEngine::getPortfolio() const { return portfolio_; }

}