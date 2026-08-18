#include "strategy/MultiAssetDeltaHedgeStrategy.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iostream>

namespace quant
{

    MultiAssetDeltaHedgeStrategy::MultiAssetDeltaHedgeStrategy(EventQueue &event_queue, const HistoricalDataFeed &data_feed, Portfolio &portfolio)
        : Strategy(event_queue, data_feed), portfolio_(portfolio) {}

    void MultiAssetDeltaHedgeStrategy::addAssetConfig(const Symbol &underlying, const DynamicOptionParams &params)
    {
        configs_[underlying] = params;
    }

    void MultiAssetDeltaHedgeStrategy::onMarketEvent(const MarketEvent &event)
    {
        const Symbol &underlying = event.getSymbol();
        auto it = configs_.find(underlying);
        if (it == configs_.end())
        {
            return;
        }

        DynamicOptionParams &config = it->second;
        double spot_price = event.getBar().close;
        uint64_t current_timestamp = event.getTimestamp();

        if (spot_price <= 0.0)
            return;

        // Mettre à jour l'historique des prix pour la volatilité
        config.price_history.push_back(spot_price);
        if (config.price_history.size() > config.vol_window_size)
        {
            config.price_history.pop_front();
        }

        // Attendre d'avoir au moins 10 bars pour calculer une volatilité réaliste
        if (config.price_history.size() < 10)
            return;

        double current_volatility = calculateRealizedVolatility(config.price_history);

        // Initialisation de l'option lors du premier bar valide
        if (config.start_timestamp == 0)
        {
            config.start_timestamp = current_timestamp;
            config.strike = spot_price * config.moneyness_ratio;
        }

        // Temps restant avant expiration (T)
        double elapsed_years = static_cast<double>(current_timestamp - config.start_timestamp) / (365.25 * 86400.0);
        double time_to_maturity = std::max(0.0001, config.initial_target_dte_years - elapsed_years);

        // --- CRÉDIT DE LA PRIME D'OPTION AU DÉBUT ---
        if (!config.option_sold)
        {
            double option_premium_per_share = calculateOptionPrice(spot_price, config.strike, time_to_maturity,
                                                                   config.risk_free_rate, current_volatility, true);

            double total_premium_collected = option_premium_per_share * 100.0; // 1 contrat = 100 actions
            portfolio_.addCash(total_premium_collected);
            config.option_sold = true;

            std::cout << "[Strategy] Option vendue pour " << underlying.ticker
                      << " | Strike: $" << config.strike
                      << " | Prime récoltée: $" << total_premium_collected << "\n";
        }

        // Calcul du Delta
        double delta = calculateDelta(spot_price, config.strike, time_to_maturity,
                                      config.risk_free_rate, current_volatility, true);

        double target_stock_position = delta * 100.0;
        double position_change = target_stock_position - config.current_position;

        // --- REBALANCEMENT BASÉ SUR LE THRESHOLD ---
        // Le seuil empêche le sur-trading (ex: 0.05 * 100 = 5 actions minimum)
        if (std::abs(position_change) >= (config.rebalance_threshold * 100.0))
        {
            SignalType signal_type = (position_change > 0.0) ? SignalType::Buy : SignalType::Sell;
            emitSignal(underlying, signal_type, std::abs(position_change), current_timestamp);
            config.current_position = target_stock_position;
        }
    }

    double MultiAssetDeltaHedgeStrategy::calculateOptionPrice(double S, double K, double T, double r, double v, bool is_call) const
    {
        double d1 = (std::log(S / K) + (r + 0.5 * v * v) * T) / (v * std::sqrt(T));
        double d2 = d1 - v * std::sqrt(T);

        double n_d1 = 0.5 * std::erfc(-d1 * 0.7071067811865475);
        double n_d2 = 0.5 * std::erfc(-d2 * 0.7071067811865475);

        if (is_call)
        {
            return S * n_d1 - K * std::exp(-r * T) * n_d2;
        }
        else
        {
            return K * std::exp(-r * T) * (1.0 - n_d2) - S * (1.0 - n_d1);
        }
    }

    double MultiAssetDeltaHedgeStrategy::calculateDelta(double S, double K, double T, double r, double v, bool is_call) const
    {
        double d1 = (std::log(S / K) + (r + 0.5 * v * v) * T) / (v * std::sqrt(T));
        double n_d1 = 0.5 * std::erfc(-d1 * 0.7071067811865475);
        return is_call ? n_d1 : (n_d1 - 1.0);
    }

    double MultiAssetDeltaHedgeStrategy::calculateRealizedVolatility(const std::deque<double> &prices) const
    {
        std::vector<double> log_returns;
        log_returns.reserve(prices.size() - 1);

        for (size_t i = 1; i < prices.size(); ++i)
        {
            log_returns.push_back(std::log(prices[i] / prices[i - 1]));
        }

        double sum = std::accumulate(log_returns.begin(), log_returns.end(), 0.0);
        double mean = sum / log_returns.size();

        double sq_sum = 0.0;
        for (double r : log_returns)
        {
            sq_sum += (r - mean) * (r - mean);
        }

        double variance = sq_sum / (log_returns.size() - 1);
        return std::sqrt(variance) * std::sqrt(252.0); // Volatilité annualisée
    }

} // namespace quant