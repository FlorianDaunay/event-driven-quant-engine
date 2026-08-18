#ifndef MULTIASSETDELTAHEDGESTRATEGY_HPP
#define MULTIASSETDELTAHEDGESTRATEGY_HPP

#include "strategy/Strategy.hpp"
#include "portfolio/Portfolio.hpp"
#include <unordered_map>
#include <deque>

namespace quant
{

    struct DynamicOptionParams
    {
        double initial_target_dte_years{0.25}; // 90 jours
        double moneyness_ratio{1.0};           // At-The-Money (ATM)
        double risk_free_rate{0.05};

        double rebalance_threshold{0.1};

        size_t vol_window_size{60};

        // États internes
        double strike{0.0};
        double current_position{0.0};
        uint64_t start_timestamp{0};
        bool option_sold{false};
        std::deque<double> price_history;
    };

    class MultiAssetDeltaHedgeStrategy : public Strategy
    {
    public:
        MultiAssetDeltaHedgeStrategy(EventQueue &event_queue, const HistoricalDataFeed &data_feed, Portfolio &portfolio);
        ~MultiAssetDeltaHedgeStrategy() override = default;

        void addAssetConfig(const Symbol &underlying, const DynamicOptionParams &params);
        void onMarketEvent(const MarketEvent &event) override;

    private:
        struct SymbolHash
        {
            std::size_t operator()(const Symbol &s) const
            {
                return std::hash<std::string>{}(s.ticker);
            }
        };

        Portfolio &portfolio_;
        std::unordered_map<Symbol, DynamicOptionParams, SymbolHash> configs_;

        double calculateDelta(double S, double K, double T, double r, double v, bool is_call) const;
        double calculateOptionPrice(double S, double K, double T, double r, double v, bool is_call) const;
        double calculateRealizedVolatility(const std::deque<double> &prices) const;
    };

}

#endif // MULTIASSETDELTAHEDGESTRATEGY_HPP