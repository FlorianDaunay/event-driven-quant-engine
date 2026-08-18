#ifndef DELTAHEDGESTRATEGY_HPP
#define DELTAHEDGESTRATEGY_HPP

#include "strategy/Strategy.hpp"

namespace quant
{

    class DeltaHedgeStrategy : public Strategy
    {
    public:
        DeltaHedgeStrategy(
            EventQueue &event_queue,
            const HistoricalDataFeed &data_feed,
            Symbol option_symbol,
            Symbol underlying_symbol,
            double strike,
            double time_to_maturity,
            double risk_free_rate,
            double volatility,
            bool is_call,
            double threshold = 0.05);

        ~DeltaHedgeStrategy() override = default;

        void onMarketEvent(const MarketEvent &event) override;

        double calculateDelta(double underlying_price) const;

    private:
        Symbol option_symbol_;
        Symbol underlying_symbol_;
        double strike_;
        double time_to_maturity_;
        double risk_free_rate_;
        double volatility_;
        bool is_call_;
        double threshold_;
        double current_underlying_position_;
        double current_delta_;

        double cumulativeNormalCDF(double value) const;
    };

}

#endif