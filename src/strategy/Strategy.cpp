#include "strategy/Strategy.hpp"

namespace quant
{

    Strategy::Strategy(EventQueue &event_queue, const HistoricalDataFeed &data_feed)
        : event_queue_(event_queue), data_feed_(data_feed) {}

    void Strategy::emitSignal(const Symbol &symbol, SignalType signal_type, double strength, uint64_t timestamp)
    {
        uint64_t signal_timestamp = timestamp;
        if (signal_timestamp == 0)
        {
            const TradeBar *bar = data_feed_.getLatestBar(symbol);
            if (bar)
            {
                signal_timestamp = bar->timestamp;
            }
        }

        auto signal_event = std::make_shared<SignalEvent>(
            signal_timestamp,
            symbol,
            signal_type,
            strength);

        event_queue_.push(signal_event);
    }

}