#include "strategy/MultiAssetDeltaHedgeStrategy.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <ctime>

namespace quant
{

    MultiAssetDeltaHedgeStrategy::MultiAssetDeltaHedgeStrategy(EventQueue &event_queue,
                                                               const HistoricalDataFeed &data_feed,
                                                               Portfolio &portfolio,
                                                               const std::string &csv_filepath)
        : Strategy(event_queue, data_feed), portfolio_(portfolio)
    {
        csv_file_.open(csv_filepath, std::ios::out | std::ios::trunc);
        if (csv_file_.is_open())
        {
            csv_file_ << "open_date,roll_date,ticker,action,expired_strike,final_price,settlement_payout,rolled_qty,option_type,new_strike,premium_collected,net_cashflow,portfolio_equity,portfolio_cash\n";
        }
    }

    MultiAssetDeltaHedgeStrategy::~MultiAssetDeltaHedgeStrategy()
    {
        if (csv_file_.is_open())
        {
            csv_file_.close();
        }
    }

    void MultiAssetDeltaHedgeStrategy::addAssetConfig(const Symbol &underlying, const DynamicOptionParams &params)
    {
        configs_[underlying] = params;
    }

    std::string MultiAssetDeltaHedgeStrategy::formatTimestamp(uint64_t timestamp_sec) const
    {
        if (timestamp_sec == 0)
            return "N/A";
        std::time_t temp = static_cast<std::time_t>(timestamp_sec);
        std::tm *t = std::gmtime(&temp);
        if (!t)
            return "N/A";

        char buffer[11];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", t);
        return std::string(buffer);
    }

    double MultiAssetDeltaHedgeStrategy::calculateDelta(double S, double K, double T, double r, double v, bool is_call) const
    {
        if (T <= 0.0001)
            return (S > K) ? (is_call ? 1.0 : -1.0) : 0.0;

        double d1 = (std::log(S / K) + (r + 0.5 * v * v) * T) / (v * std::sqrt(T));
        double cdf = 0.5 * std::erfc(-d1 * M_SQRT1_2);
        return is_call ? cdf : (cdf - 1.0);
    }

    double MultiAssetDeltaHedgeStrategy::calculateOptionPrice(double S, double K, double T, double r, double v, bool is_call) const
    {
        if (T <= 0.0001)
            return std::max(0.0, is_call ? (S - K) : (K - S));

        double d1 = (std::log(S / K) + (r + 0.5 * v * v) * T) / (v * std::sqrt(T));
        double d2 = d1 - v * std::sqrt(T);

        double N_d1 = 0.5 * std::erfc(-d1 * M_SQRT1_2);
        double N_d2 = 0.5 * std::erfc(-d2 * M_SQRT1_2);

        if (is_call)
        {
            return S * N_d1 - K * std::exp(-r * T) * N_d2;
        }
        else
        {
            return K * std::exp(-r * T) * (1.0 - N_d2) - S * (1.0 - N_d1);
        }
    }

    double MultiAssetDeltaHedgeStrategy::calculateRealizedVolatility(const std::deque<double> &prices) const
    {
        if (prices.size() < 2)
            return 0.20; // Default fallback volatility (20%)

        std::vector<double> returns;
        returns.reserve(prices.size() - 1);

        for (size_t i = 1; i < prices.size(); ++i)
        {
            returns.push_back(std::log(prices[i] / prices[i - 1]));
        }

        double sum = 0.0;
        for (double r : returns)
            sum += r;
        double mean = sum / returns.size();

        double sq_diff_sum = 0.0;
        for (double r : returns)
            sq_diff_sum += (r - mean) * (r - mean);

        double std_dev = std::sqrt(sq_diff_sum / (returns.size() - 1));
        return std_dev * std::sqrt(252.0); // Annualized volatility
    }

    void MultiAssetDeltaHedgeStrategy::settleOptionExpiration(const Symbol &symbol, DynamicOptionParams &params, double current_price, uint64_t current_ts)
    {
        double payoff = std::max(0.0, current_price - params.strike) * params.contract_multiplier * std::abs(params.option_quantity);

        // Deduct payout from portfolio cash if ITM
        if (payoff > 0.0)
        {
            portfolio_.addCash(-payoff);
        }

        uint64_t open_ts = params.start_timestamp;
        double expired_strike = params.strike;
        int rolled_qty = static_cast<int>(std::abs(params.option_quantity));

        // Reset option state
        params.option_sold = false;
        params.strike = 0.0;
        params.option_quantity = 0.0;

        // Automatically sell a new option right away (Roll)
        double vol = calculateRealizedVolatility(params.price_history);
        bool roll_success = trySellOption(symbol, params, current_price, current_ts, vol);

        double new_strike = roll_success ? params.strike : 0.0;
        double premium_collected = roll_success ? params.option_premium_collected : 0.0;

        // Capture total portfolio equity and cash at the time of roll
        double current_equity = portfolio_.getTotalEquity();
        double current_cash = portfolio_.getCash();

        // Consolidated single-line log and CSV export with portfolio state
        logAndExportRoll(open_ts, current_ts, symbol.ticker, expired_strike, current_price, -payoff, rolled_qty, new_strike, premium_collected, current_equity, current_cash);
    }

    void MultiAssetDeltaHedgeStrategy::logAndExportRoll(uint64_t open_ts, uint64_t roll_ts, const std::string &ticker,
                                                        double expired_strike, double final_price, double settlement_payout,
                                                        int rolled_qty, double new_strike, double premium_collected,
                                                        double current_equity, double current_cash)
    {
        std::string open_date = formatTimestamp(open_ts);
        std::string roll_date = formatTimestamp(roll_ts);
        double net_cashflow = premium_collected + settlement_payout; // settlement_payout is <= 0

        // 1. Single-line Terminal Console Output
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "[" << open_date << " -> " << roll_date << "] ROLL | " << std::left << std::setw(6) << ticker
                  << " | EXPIRED [Strike: $" << expired_strike
                  << " | Final: $" << final_price
                  << " | Payout: -$" << std::abs(settlement_payout) << "]"
                  << " -> ROLLED [Qty: " << rolled_qty << "x Call"
                  << " | New Strike: $" << new_strike
                  << " | Premium: +$" << premium_collected << "]"
                  << " | Net Cashflow: " << (net_cashflow >= 0.0 ? "+$" : "-$") << std::abs(net_cashflow)
                  << " | Equity: $" << current_equity << "\n";

        // 2. Export row into CSV
        if (csv_file_.is_open())
        {
            csv_file_ << open_date << ","
                      << roll_date << ","
                      << ticker << ","
                      << "SETTLE_AND_ROLL,"
                      << expired_strike << ","
                      << final_price << ","
                      << settlement_payout << ","
                      << rolled_qty << ","
                      << "Call,"
                      << new_strike << ","
                      << premium_collected << ","
                      << net_cashflow << ","
                      << current_equity << ","
                      << current_cash << "\n";
        }
    }

    bool MultiAssetDeltaHedgeStrategy::trySellOption(const Symbol &, DynamicOptionParams &params, double current_price, uint64_t current_ts, double vol)
    {
        size_t num_assets = configs_.empty() ? 1 : configs_.size();
        double total_target_allocation = 0.90;
        double capital_per_asset = (portfolio_.getTotalEquity() * total_target_allocation) / num_assets;

        double call_notional_per_contract = current_price * params.contract_multiplier;
        int num_contracts = static_cast<int>(capital_per_asset / call_notional_per_contract);

        double available_cash = portfolio_.getCash();
        double min_reserved_cash = available_cash * params.cash_reserve_ratio;
        double usable_cash = available_cash - min_reserved_cash;

        double margin_per_contract = call_notional_per_contract * params.margin_req_ratio;
        int max_affordable_contracts = static_cast<int>(usable_cash / margin_per_contract);

        num_contracts = std::min(num_contracts, max_affordable_contracts);

        if (num_contracts <= 0)
            return false;

        params.strike = current_price * params.moneyness_ratio;
        params.start_timestamp = current_ts;

        uint64_t dte_seconds = static_cast<uint64_t>(params.initial_target_dte_years * 365.25 * 86400.0);
        params.expiry_timestamp = current_ts + dte_seconds;

        params.option_quantity = -num_contracts;

        double opt_price = calculateOptionPrice(current_price, params.strike, params.initial_target_dte_years,
                                                params.risk_free_rate, vol, true);
        double total_premium = opt_price * params.contract_multiplier * num_contracts;

        params.option_premium_collected = total_premium;
        portfolio_.addCash(total_premium);
        params.option_sold = true;

        return true;
    }

    void MultiAssetDeltaHedgeStrategy::onMarketEvent(const MarketEvent &event)
    {
        const Symbol &symbol = event.getSymbol();
        const TradeBar &bar = event.getBar();

        auto it = configs_.find(symbol);
        if (it == configs_.end())
            return;

        auto &params = it->second;

        double current_price = bar.close;
        uint64_t current_ts = bar.timestamp;

        params.price_history.push_back(current_price);
        if (params.price_history.size() > params.vol_window_size)
        {
            params.price_history.pop_front();
        }

        double vol = calculateRealizedVolatility(params.price_history);

        // Settlement Check: If active option expired, settle & roll it
        if (params.option_sold && current_ts >= params.expiry_timestamp)
        {
            settleOptionExpiration(symbol, params, current_price, current_ts);
        }

        // Rolling Check: Initial setup if no active option exists
        if (!params.option_sold)
        {
            trySellOption(symbol, params, current_price, current_ts, vol);
        }

        // Dynamic Delta Hedging
        if (params.option_sold)
        {
            double remaining_years = (params.expiry_timestamp > current_ts) ? static_cast<double>(params.expiry_timestamp - current_ts) / (365.25 * 86400.0) : 0.0;

            double option_delta = calculateDelta(current_price, params.strike, remaining_years,
                                                 params.risk_free_rate, vol, true);

            double target_stock_qty = -1.0 * (option_delta * params.option_quantity * params.contract_multiplier);
            double delta_diff = target_stock_qty - params.current_position;

            if (std::abs(delta_diff) >= (params.rebalance_threshold * params.contract_multiplier))
            {
                OrderSide side = (delta_diff > 0) ? OrderSide::Buy : OrderSide::Sell;
                event_queue_.push(
                    std::make_unique<OrderEvent>(
                        current_ts,
                        symbol,
                        OrderType::Market,
                        side,
                        std::abs(delta_diff)));

                params.current_position = target_stock_qty;
            }
        }
    }

}