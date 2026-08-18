#ifndef HISTORICALDATAFEED_HPP
#define HISTORICALDATAFEED_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "core/Types.hpp"
#include "core/Event.hpp"
#include "core/EventQueue.hpp"

namespace quant
{

    class HistoricalDataFeed
    {
    public:
        HistoricalDataFeed(EventQueue &event_queue);
        ~HistoricalDataFeed() = default;

        HistoricalDataFeed(const HistoricalDataFeed &) = delete;
        HistoricalDataFeed &operator=(const HistoricalDataFeed &) = delete;

        bool loadCSV(const Symbol &symbol, const std::string &filepath);
        bool loadMultipleCSV(const std::vector<std::pair<Symbol, std::string>> &symbol_filepaths);
        bool step();
        bool hasNext() const;
        void reset();

        const TradeBar *getLatestBar(const Symbol &symbol) const;

    private:
        EventQueue &event_queue_;
        std::map<Symbol, std::vector<TradeBar>> historical_data_;
        std::map<Symbol, size_t> current_indices_;
        uint64_t current_timestamp_;

        uint64_t parseTimestamp(const std::string &datetime_str) const;
    };

}

#endif