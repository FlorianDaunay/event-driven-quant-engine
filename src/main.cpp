#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>

#include "engine/BacktestEngine.hpp"
#include "portfolio/Portfolio.hpp"
#include "strategy/Strategy.hpp"
#include "strategy/DeltaHedgeStrategy.hpp"
#include "strategy/MultiAssetDeltaHedgeStrategy.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// CONFIGURATION SETUP
// ============================================================================
enum class StrategyType
{
    SingleAsset,
    MultiAsset
};

struct SimulationConfig
{
    // Mode Selection
    StrategyType strategy_type = StrategyType::MultiAsset;

    // Ticker Universe (First ticker will be used if StrategyType::SingleAsset is selected)
    std::vector<std::string> tickers = {"AAPL", "MSFT", "GOOGL", "MA", "MC.PA"};

    // Execution & Engine Settings
    double initial_cash = 10000000.0;
    double commission_per_share = 0.005;
    double slippage_pct = 0.0002;

    // Strategy Parameters
    double initial_target_dte_years = 0.25;
    double moneyness_ratio = 1.0;
    int vol_window_size = 60;
    double rebalance_threshold = 0.01; // 5% rebalance threshold
};

// ============================================================================
// STRATEGY FACTORY
// ============================================================================
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

        std::cout << "[Strategy] Instantiating Single-Asset DeltaHedgeStrategy ("
                  << underlying_symbol.ticker << ")...\n";

        // Define explicit option contract parameters for the single-asset strategy
        quant::Symbol option_symbol(underlying_symbol.ticker, quant::SecurityType::Option);
        double strike_price = 150.0; // Set your initial target strike
        double dte_years = config.initial_target_dte_years;
        double risk_free_rate = 0.05;       // e.g., 5%
        double estimated_volatility = 0.20; // e.g., 20% IV
        bool is_call = true;

        return std::make_unique<quant::DeltaHedgeStrategy>(
            event_queue,
            engine.getDataFeed(),
            underlying_symbol,
            option_symbol,
            strike_price,
            dte_years,
            risk_free_rate,
            estimated_volatility,
            is_call,
            config.rebalance_threshold);
    }
    else
    {
        std::cout << "[Strategy] Instantiating MultiAssetDeltaHedgeStrategy...\n";

        auto strategy = std::make_unique<quant::MultiAssetDeltaHedgeStrategy>(
            event_queue,
            engine.getDataFeed(),
            portfolio);

        quant::DynamicOptionParams default_params;
        default_params.initial_target_dte_years = config.initial_target_dte_years;
        default_params.moneyness_ratio = config.moneyness_ratio;
        default_params.vol_window_size = config.vol_window_size;
        default_params.rebalance_threshold = config.rebalance_threshold;

        for (const auto &[symbol, path] : datasets)
        {
            strategy->addAssetConfig(symbol, default_params);
        }

        return strategy;
    }
}

// ============================================================================
// MAIN EXECUTION
// ============================================================================
int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    SimulationConfig config;

    std::cout << "====================================================\n";
    std::cout << "   QUANT ENGINE C++17 - EVENT-DRIVEN BACKTESTER    \n";
    std::cout << "====================================================\n\n";

    // 1. Prepare Dataset Pairs based on Config
    std::vector<std::pair<quant::Symbol, std::string>> datasets;

    if (config.strategy_type == StrategyType::SingleAsset)
    {
        // Use only the primary ticker for single asset mode
        const std::string &primary_ticker = config.tickers.front();
        datasets.emplace_back(
            quant::Symbol(primary_ticker, quant::SecurityType::Equity),
            "data/" + primary_ticker + ".csv");
    }
    else
    {
        // Load full ticker universe for multi asset mode
        datasets.reserve(config.tickers.size());
        for (const auto &ticker : config.tickers)
        {
            datasets.emplace_back(
                quant::Symbol(ticker, quant::SecurityType::Equity),
                "data/" + ticker + ".csv");
        }
    }

    // 2. Initialize Engine
    quant::BacktestEngine engine(config.initial_cash, config.commission_per_share, config.slippage_pct);

    std::cout << "[1/4] Loading historical data for " << datasets.size() << " ticker(s)...\n";
    if (!engine.loadData(datasets))
    {
        std::cerr << "Error: Failed to load dataset CSV files.\n";
        return 1;
    }
    std::cout << "     -> Data loaded successfully.\n\n";

    // 3. Attach Strategy via Factory
    std::cout << "[2/4] Setting up Strategy...\n";
    engine.setStrategy(createStrategy(config, engine, datasets));
    std::cout << "     -> Strategy registered successfully.\n\n";

    // 4. Run Backtest
    std::cout << "[3/4] Running backtest simulation...\n";
    try
    {
        quant::BacktestResults results = engine.run();
        std::cout << "     -> Simulation completed.\n\n";

        // 5. Output Results Summary
        std::cout << "====================================================\n";
        std::cout << "               BACKTEST RESULTS SUMMARY             \n";
        std::cout << "====================================================\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << " Initial Cash                  : $" << results.initial_cash << "\n";
        std::cout << " Final Portfolio Equity        : $" << results.final_equity << "\n";
        std::cout << " Total Realized PnL            : $" << results.total_realized_pnl << "\n";
        std::cout << " Total Unrealized PnL          : $" << results.total_unrealized_pnl << "\n";
        std::cout << " Total Return                  : " << results.total_return_pct << " %\n";
        std::cout << " Processed Events              : " << results.total_events_processed << "\n";
        std::cout << "====================================================\n\n";

        // 6. Asset Position Breakdown
        std::cout << "--- FINAL ASSET POSITIONS BREAKDOWN ---\n";
        for (const auto &[symbol, pos] : engine.getPortfolio().getAllPositions())
        {
            std::cout << " Ticker: " << std::left << std::setw(6) << symbol.ticker
                      << " | Qty: " << std::right << std::setw(8) << pos.quantity
                      << " | Avg Price: $" << std::setw(8) << pos.average_price
                      << " | Realized PnL: $" << std::setw(10) << pos.realized_pnl << "\n";
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