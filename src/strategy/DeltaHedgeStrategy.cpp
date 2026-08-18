#include "strategy/DeltaHedgeStrategy.hpp"
#include <cmath>

namespace quant
{

    DeltaHedgeStrategy::DeltaHedgeStrategy(
        EventQueue &event_queue,
        const HistoricalDataFeed &data_feed,
        Symbol option_symbol,
        Symbol underlying_symbol,
        double strike,
        double time_to_maturity,
        double risk_free_rate,
        double volatility,
        bool is_call,
        double threshold) : Strategy(event_queue, data_feed),
                            option_symbol_(option_symbol),
                            underlying_symbol_(underlying_symbol),
                            strike_(strike),
                            time_to_maturity_(time_to_maturity),
                            risk_free_rate_(risk_free_rate),
                            volatility_(volatility),
                            is_call_(is_call),
                            threshold_(threshold),
                            current_underlying_position_(0.0),
                            current_delta_(0.0) {}

    void DeltaHedgeStrategy::onMarketEvent(const MarketEvent &event)
    {
        if (!(event.getSymbol() == underlying_symbol_))
        {
            return;
        }

        double underlying_price = event.getBar().close;
        double new_delta = calculateDelta(underlying_price);
        double target_position = -new_delta;
        double position_diff = target_position - current_underlying_position_;

        if (std::abs(position_diff) >= threshold_)
        {
            SignalType signal_type = (position_diff > 0.0) ? SignalType::Buy : SignalType::Sell;
            emitSignal(underlying_symbol_, signal_type, std::abs(position_diff), event.getTimestamp());
            current_underlying_position_ = target_position;
            current_delta_ = new_delta;
        }
    }

    double DeltaHedgeStrategy::calculateDelta(double underlying_price) const
    {
        if (underlying_price <= 0.0 || volatility_ <= 0.0 || time_to_maturity_ <= 0.0)
        {
            return 0.0;
        }

        double d1 = (std::log(underlying_price / strike_) +
                     (risk_free_rate_ + 0.5 * volatility_ * volatility_) * time_to_maturity_) /
                    (volatility_ * std::sqrt(time_to_maturity_));

        if (is_call_)
        {
            return cumulativeNormalCDF(d1);
        }
        else
        {
            return cumulativeNormalCDF(d1) - 1.0;
        }
    }

    double DeltaHedgeStrategy::cumulativeNormalCDF(double value) const
    {
        return 0.5 * std::erfc(-value * 0.70710678118654752440084436210485);
    }

}