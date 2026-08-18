#include "data/HistoricalDataFeed.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace quant
{

    HistoricalDataFeed::HistoricalDataFeed(EventQueue &event_queue)
        : event_queue_(event_queue), current_timestamp_(0) {}

    bool HistoricalDataFeed::loadCSV(const Symbol &symbol, const std::string &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            return false;
        }

        std::string line;
        std::getline(file, line);

        std::vector<TradeBar> bars;
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            std::stringstream ss(line);
            std::string datetime_str, open_str, high_str, low_str, close_str, volume_str;

            std::getline(ss, datetime_str, ',');
            std::getline(ss, open_str, ',');
            std::getline(ss, high_str, ',');
            std::getline(ss, low_str, ',');
            std::getline(ss, close_str, ',');
            std::getline(ss, volume_str, ',');

            try
            {
                TradeBar bar;
                bar.timestamp = parseTimestamp(datetime_str);
                bar.open = std::stod(open_str);
                bar.high = std::stod(high_str);
                bar.low = std::stod(low_str);
                bar.close = std::stod(close_str);
                bar.volume = std::stod(volume_str);

                bars.push_back(bar);
            }
            catch (...)
            {
                continue;
            }
        }

        std::sort(bars.begin(), bars.end(), [](const TradeBar &a, const TradeBar &b)
                  { return a.timestamp < b.timestamp; });

        historical_data_[symbol] = std::move(bars);
        current_indices_[symbol] = 0;

        return !historical_data_[symbol].empty();
    }

    bool HistoricalDataFeed::loadMultipleCSV(const std::vector<std::pair<Symbol, std::string>> &symbol_filepaths)
    {
        bool all_success = true;
        for (const auto &[symbol, filepath] : symbol_filepaths)
        {
            if (!loadCSV(symbol, filepath))
            {
                all_success = false;
            }
        }
        return all_success;
    }

    bool HistoricalDataFeed::step()
    {
        if (!hasNext())
        {
            return false;
        }

        uint64_t next_timestamp = UINT64_MAX;
        for (const auto &[symbol, bars] : historical_data_)
        {
            size_t idx = current_indices_[symbol];
            if (idx < bars.size())
            {
                if (bars[idx].timestamp < next_timestamp)
                {
                    next_timestamp = bars[idx].timestamp;
                }
            }
        }

        if (next_timestamp == UINT64_MAX)
        {
            return false;
        }

        current_timestamp_ = next_timestamp;

        for (auto &[symbol, bars] : historical_data_)
        {
            size_t idx = current_indices_[symbol];
            if (idx < bars.size() && bars[idx].timestamp == current_timestamp_)
            {
                auto event = std::make_shared<MarketEvent>(current_timestamp_, symbol, bars[idx]);
                event_queue_.push(event);
                current_indices_[symbol]++;
            }
        }

        return true;
    }

    bool HistoricalDataFeed::hasNext() const
    {
        for (const auto &[symbol, bars] : historical_data_)
        {
            if (current_indices_.at(symbol) < bars.size())
            {
                return true;
            }
        }
        return false;
    }

    void HistoricalDataFeed::reset()
    {
        for (auto &[symbol, idx] : current_indices_)
        {
            idx = 0;
        }
        current_timestamp_ = 0;
    }

    const TradeBar *HistoricalDataFeed::getLatestBar(const Symbol &symbol) const
    {
        auto data_it = historical_data_.find(symbol);
        auto idx_it = current_indices_.find(symbol);

        if (data_it == historical_data_.end() || idx_it == current_indices_.end())
        {
            return nullptr;
        }

        size_t current_idx = idx_it->second;
        if (current_idx == 0)
        {
            return nullptr;
        }

        return &data_it->second[current_idx - 1];
    }

    uint64_t HistoricalDataFeed::parseTimestamp(const std::string &datetime_str) const
    {
        if (datetime_str.find('-') != std::string::npos || datetime_str.find('/') != std::string::npos)
        {
            int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
            char delim1, delim2;

            std::stringstream ss(datetime_str);
            ss >> year >> delim1 >> month >> delim2 >> day;

            if (ss.peek() == ' ' || ss.peek() == 'T')
            {
                ss.ignore();
                char time_delim;
                ss >> hour >> time_delim >> min >> time_delim >> sec;
            }

            uint64_t formatted = static_cast<uint64_t>(year) * 10000000000ULL +
                                 static_cast<uint64_t>(month) * 100000000ULL +
                                 static_cast<uint64_t>(day) * 1000000ULL +
                                 static_cast<uint64_t>(hour) * 10000ULL +
                                 static_cast<uint64_t>(min) * 100ULL +
                                 static_cast<uint64_t>(sec);
            return formatted;
        }

        try
        {
            return std::stoull(datetime_str);
        }
        catch (...)
        {
            return 0;
        }
    }

}