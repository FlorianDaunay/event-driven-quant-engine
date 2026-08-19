#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>

#include "engine/BacktestEngine.hpp"
#include "portfolio/Portfolio.hpp"
#include "strategy/Strategy.hpp"
#include "strategy/DeltaHedgeStrategy.hpp"
#include "strategy/MultiAssetDeltaHedgeStrategy.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

enum class StrategyType
{
    SingleAsset,
    MultiAsset
};

struct SimulationConfig
{
    StrategyType strategy_type = StrategyType::MultiAsset;
    std::vector<std::string> tickers = {"AAPL", "MSFT", "GOOGL", "AMZN", "NVDA", "META", "TSLA", "JNJ", "JPM", "MA", "MC.PA"};

    std::string start_date = "2016-01-01";
    std::string end_date = "2026-01-01";

    double initial_cash = 10000000.0;
    double commission_per_share = 0.005;
    double slippage_pct = 0.0002;

    double initial_target_dte_years = 0.25;
    double moneyness_ratio = 1.0;
    int vol_window_size = 60;
    double rebalance_threshold = 0.05;

    double cash_reserve_ratio = 0.25;
    double margin_req_ratio = 0.10;
};

// Convertit une chaîne "YYYY-MM-DD" en timestamp POSIX (secondes)
uint64_t dateToTimestamp(const std::string &date_str)
{
    if (date_str.empty())
        return 0;
    std::tm tm = {};
    std::stringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail())
        return 0;

    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;

    std::time_t t = std::mktime(&tm);
    return (t == -1) ? 0 : static_cast<uint64_t>(t);
}

std::string formatTimestamp(uint64_t timestamp_sec)
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

std::unique_ptr<quant::Strategy> createStrategy(
    const SimulationConfig &config,
    quant::BacktestEngine &engine,
    const std::vector<std::pair<quant::Symbol, std::string>> &datasets)
{
    auto &event_queue = const_cast<quant::EventQueue &>(engine.getEventQueue());
    auto &portfolio = const_cast<quant::Portfolio &>(engine.getPortfolio());

    if (config.strategy_type == StrategyType::SingleAsset)
    {
        const auto &underlying_symbol = datasets.front().first;
        quant::Symbol option_symbol(underlying_symbol.ticker, quant::SecurityType::Option);

        return std::make_unique<quant::DeltaHedgeStrategy>(
            event_queue,
            engine.getDataFeed(),
            underlying_symbol,
            option_symbol,
            150.0,
            config.initial_target_dte_years,
            0.05,
            0.20,
            true,
            config.rebalance_threshold);
    }
    else
    {
        auto strategy = std::make_unique<quant::MultiAssetDeltaHedgeStrategy>(
            event_queue,
            engine.getDataFeed(),
            portfolio);

        quant::DynamicOptionParams default_params;
        default_params.initial_target_dte_years = config.initial_target_dte_years;
        default_params.moneyness_ratio = config.moneyness_ratio;
        default_params.vol_window_size = config.vol_window_size;
        default_params.rebalance_threshold = config.rebalance_threshold;
        default_params.cash_reserve_ratio = config.cash_reserve_ratio;
        default_params.margin_req_ratio = config.margin_req_ratio;

        for (const auto &[symbol, path] : datasets)
        {
            strategy->addAssetConfig(symbol, default_params);
        }

        return strategy;
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    SimulationConfig config;

    std::cout << "====================================================\n";
    std::cout << "    QUANT ENGINE C++17 - EVENT-DRIVEN BACKTESTER    \n";
    std::cout << "====================================================\n\n";

    std::vector<std::pair<quant::Symbol, std::string>> datasets;
    for (const auto &ticker : config.tickers)
    {
        datasets.emplace_back(
            quant::Symbol(ticker, quant::SecurityType::Equity),
            "data/" + ticker + ".csv");
    }

    quant::BacktestEngine engine(config.initial_cash, config.commission_per_share, config.slippage_pct);

    // Application des filtres de dates de début et de fin
    if (!config.start_date.empty())
    {
        engine.setStartDate(dateToTimestamp(config.start_date));
    }
    if (!config.end_date.empty())
    {
        engine.setEndDate(dateToTimestamp(config.end_date));
    }

    std::cout << "[1/4] Loading historical data for " << datasets.size() << " ticker(s)...\n";
    if (!engine.loadData(datasets))
    {
        std::cerr << "Error: Failed to load dataset CSV files.\n";
        return 1;
    }
    std::cout << "     -> Data loaded successfully.\n\n";

    std::cout << "[2/4] Setting up Strategy...\n";
    engine.setStrategy(createStrategy(config, engine, datasets));
    std::cout << "     -> Strategy registered successfully.\n\n";

    std::cout << "[3/4] Running backtest simulation...\n";
    try
    {
        quant::BacktestResults results = engine.run();
        std::cout << "     -> Simulation completed.\n\n";

        double duration_years = 0.0;
        double annualized_return_pct = 0.0;

        if (results.start_timestamp > 0 && results.end_timestamp > results.start_timestamp)
        {
            constexpr double seconds_per_year = 365.25 * 86400.0;
            duration_years = static_cast<double>(results.end_timestamp - results.start_timestamp) / seconds_per_year;

            if (duration_years > 0.0 && results.final_equity > 0.0)
            {
                double cagr = std::pow(results.final_equity / results.initial_cash, 1.0 / duration_years) - 1.0;
                annualized_return_pct = cagr * 100.0;
            }
        }

        std::cout << "====================================================\n";
        std::cout << "               BACKTEST RESULTS SUMMARY             \n";
        std::cout << "====================================================\n";
        std::cout << " Start Date               : " << formatTimestamp(results.start_timestamp) << "\n";
        std::cout << " End Date                 : " << formatTimestamp(results.end_timestamp) << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << " Duration                 : " << duration_years << " Years\n";
        std::cout << "----------------------------------------------------\n";
        std::cout << " Initial Cash             : $" << results.initial_cash << "\n";
        std::cout << " Final Portfolio Equity   : $" << results.final_equity << "\n";
        std::cout << " Total Realized PnL       : $" << results.total_realized_pnl << "\n";
        std::cout << " Total Unrealized PnL     : $" << results.total_unrealized_pnl << "\n";
        std::cout << " Total Return             : " << results.total_return_pct << " %\n";
        std::cout << " Annualized Return (CAGR) : " << annualized_return_pct << " %\n";
        std::cout << " Processed Events         : " << results.total_events_processed << "\n";
        std::cout << "====================================================\n\n";

        std::cout << "--- FINAL ASSET POSITIONS BREAKDOWN ---\n";
        for (const auto &[symbol, pos] : engine.getPortfolio().getAllPositions())
        {
            std::cout << " Ticker: " << std::left << std::setw(6) << symbol.ticker
                      << " | Qty: " << std::right << std::setw(10) << pos.quantity
                      << " | Avg Price: $" << std::setw(8) << pos.average_price
                      << " | Realized PnL: $" << std::setw(11) << pos.realized_pnl << "\n";
        }
        std::cout << "====================================================\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal execution error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}