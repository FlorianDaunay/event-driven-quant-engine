#ifndef MULTIASSETDELTAHEDGESTRATEGY_HPP
#define MULTIASSETDELTAHEDGESTRATEGY_HPP

#include "strategy/Strategy.hpp"
#include "portfolio/Portfolio.hpp"
#include <unordered_map>
#include <deque>
#include <fstream>
#include <string>

namespace quant
{

    struct DynamicOptionParams
    {
        // --- Strategy Parameters ---
        double initial_target_dte_years{0.25}; // Target expiry (e.g. 90 days = 0.25y)
        double moneyness_ratio{1.0};           // 1.0 = ATM, 1.05 = 5% OTM
        double risk_free_rate{0.05};
        double rebalance_threshold{0.1}; // Rebalance when delta shifts by 0.10
        size_t vol_window_size{60};

        // --- Risk & Sizing Protection ---
        double cash_reserve_ratio{0.15}; // Keep at least 15% cash safe
        double margin_req_ratio{0.20};   // Margin required per short contract (20%)
        int contract_multiplier{100};    // Shares per contract

        // --- Internal State Trackers ---
        double strike{0.0};
        double current_position{0.0};         // Current share hedge quantity
        uint64_t start_timestamp{0};          // Contract open timestamp
        uint64_t expiry_timestamp{0};         // Expiration timestamp
        double option_quantity{0.0};          // Number of contracts sold (e.g., -1.0)
        double option_premium_collected{0.0}; // Total premium received for open contract
        bool option_sold{false};

        std::deque<double> price_history;
    };

    class MultiAssetDeltaHedgeStrategy : public Strategy
    {
    public:
        MultiAssetDeltaHedgeStrategy(EventQueue &event_queue,
                                     const HistoricalDataFeed &data_feed,
                                     Portfolio &portfolio,
                                     const std::string &csv_filepath = "trades.csv");
        ~MultiAssetDeltaHedgeStrategy() override;

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
        std::ofstream csv_file_;

        // Black-Scholes & Pricing utilities
        double calculateDelta(double S, double K, double T, double r, double v, bool is_call) const;
        double calculateOptionPrice(double S, double K, double T, double r, double v, bool is_call) const;
        double calculateRealizedVolatility(const std::deque<double> &prices) const;

        // Rolling, Risk & Logging helpers
        void settleOptionExpiration(const Symbol &symbol, DynamicOptionParams &params, double current_price, uint64_t current_ts);
        bool trySellOption(const Symbol &symbol, DynamicOptionParams &params, double current_price, uint64_t current_ts, double vol);

        std::string formatTimestamp(uint64_t timestamp_sec) const;
        void logAndExportRoll(uint64_t open_ts, uint64_t roll_ts, const std::string &ticker,
                              double expired_strike, double final_price, double settlement_payout,
                              int rolled_qty, double new_strike, double premium_collected,
                              double current_equity, double current_cash);
    };

}

#endif // MULTIASSETDELTAHEDGESTRATEGY_HPP