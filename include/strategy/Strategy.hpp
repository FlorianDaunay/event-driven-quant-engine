#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include <memory>
#include "core/Event.hpp"
#include "core/EventQueue.hpp"
#include "data/HistoricalDataFeed.hpp"

namespace quant
{

    class Strategy
    {
    public:
        Strategy(EventQueue &event_queue, const HistoricalDataFeed &data_feed);
        virtual ~Strategy() = default;

        Strategy(const Strategy &) = delete;
        Strategy &operator=(const Strategy &) = delete;

        virtual void onMarketEvent(const MarketEvent &event) = 0;

    protected:
        EventQueue &event_queue_;
        const HistoricalDataFeed &data_feed_;

        void emitSignal(const Symbol &symbol, SignalType signal_type, double strength = 1.0, uint64_t timestamp = 0);
    };

}

#endif