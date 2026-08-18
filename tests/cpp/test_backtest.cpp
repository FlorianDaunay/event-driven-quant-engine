#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <memory>
#include <vector>

#include "engine/BacktestEngine.hpp"
#include "strategy/Strategy.hpp"
#include "strategy/MultiAssetDeltaHedgeStrategy.hpp"

namespace quant
{

    class BacktestTest : public ::testing::Test
    {
    protected:
        std::string aapl_csv_path = "test_aapl.csv";
        std::string msft_csv_path = "test_msft.csv";

        Symbol aapl{"AAPL", SecurityType::Equity};
        Symbol msft{"MSFT", SecurityType::Equity};

        void SetUp() override
        {
            std::ofstream aapl_file(aapl_csv_path);
            aapl_file << "timestamp,open,high,low,close,volume\n";
            aapl_file << "1609459200,100.0,105.0,99.0,102.0,1000\n";
            aapl_file << "1609545600,102.0,108.0,101.0,107.0,1200\n";
            aapl_file << "1609632000,107.0,110.0,104.0,105.0,1100\n";
            aapl_file << "1609718400,105.0,109.0,103.0,108.0,1300\n";
            aapl_file << "1609804800,108.0,112.0,106.0,110.0,1400\n";
            aapl_file << "1609891200,110.0,115.0,108.0,112.0,1500\n";
            aapl_file << "1609977600,112.0,116.0,110.0,114.0,1600\n";
            aapl_file << "1610064000,114.0,118.0,112.0,115.0,1700\n";
            aapl_file << "1610150400,115.0,119.0,113.0,117.0,1800\n";
            aapl_file << "1610236800,117.0,121.0,115.0,119.0,1900\n";
            aapl_file << "1610323200,119.0,122.0,117.0,120.0,2000\n";
            aapl_file.close();

            std::ofstream msft_file(msft_csv_path);
            msft_file << "timestamp,open,high,low,close,volume\n";
            msft_file << "1609459200,200.0,205.0,198.0,202.0,1500\n";
            msft_file << "1609545600,202.0,206.0,200.0,204.0,1600\n";
            msft_file << "1609632000,204.0,208.0,202.0,206.0,1400\n";
            msft_file << "1609718400,206.0,210.0,204.0,208.0,1500\n";
            msft_file << "1609804800,208.0,212.0,206.0,210.0,1600\n";
            msft_file << "1609891200,210.0,214.0,208.0,212.0,1700\n";
            msft_file << "1609977600,212.0,216.0,210.0,214.0,1800\n";
            msft_file << "1610064000,214.0,218.0,212.0,215.0,1900\n";
            msft_file << "1610150400,215.0,219.0,213.0,217.0,2000\n";
            msft_file << "1610236800,217.0,221.0,215.0,219.0,2100\n";
            msft_file << "1610323200,219.0,222.0,217.0,220.0,2200\n";
            msft_file.close();
        }

        void TearDown() override
        {
            std::remove(aapl_csv_path.c_str());
            std::remove(msft_csv_path.c_str());
        }
    };

    class SimpleBuySellStrategy : public Strategy
    {
    public:
        SimpleBuySellStrategy(EventQueue &event_queue, const HistoricalDataFeed &data_feed, const Symbol &sym)
            : Strategy(event_queue, data_feed), symbol_(sym), step_(0) {}

        void onMarketEvent(const MarketEvent &event) override
        {
            if (!(event.getSymbol() == symbol_))
                return;

            if (step_ == 0)
            {
                emitSignal(symbol_, SignalType::Buy, 10.0, event.getTimestamp());
            }
            else if (step_ == 1)
            {
                emitSignal(symbol_, SignalType::Sell, 10.0, event.getTimestamp());
            }
            step_++;
        }

    private:
        Symbol symbol_;
        int step_;
    };

    TEST_F(BacktestTest, SingleAssetEventLoopAndPnL)
    {
        double initial_cash = 10000.0;
        double commission = 0.0;
        double slippage = 0.0;

        BacktestEngine engine(initial_cash, commission, slippage);
        ASSERT_TRUE(engine.loadData(aapl, aapl_csv_path));

        auto strategy = std::make_unique<SimpleBuySellStrategy>(
            const_cast<EventQueue &>(engine.getEventQueue()),
            engine.getDataFeed(),
            aapl);

        engine.setStrategy(std::move(strategy));

        BacktestResults results = engine.run();

        EXPECT_EQ(results.initial_cash, 10000.0);
        EXPECT_DOUBLE_EQ(results.total_realized_pnl, 50.0);
        EXPECT_DOUBLE_EQ(results.total_unrealized_pnl, 0.0);
        EXPECT_DOUBLE_EQ(results.final_equity, 10050.0);
        EXPECT_GT(results.total_events_processed, 0);
    }

    TEST_F(BacktestTest, MultiAssetDeltaHedgeLoop)
    {
        double initial_cash = 1000000.0;
        double commission = 0.0;
        double slippage = 0.0;

        BacktestEngine engine(initial_cash, commission, slippage);

        std::vector<std::pair<Symbol, std::string>> datasets = {
            {aapl, aapl_csv_path},
            {msft, msft_csv_path}};

        ASSERT_TRUE(engine.loadData(datasets));

        auto strategy = std::make_unique<MultiAssetDeltaHedgeStrategy>(
            const_cast<EventQueue &>(engine.getEventQueue()),
            engine.getDataFeed(),
            const_cast<Portfolio &>(engine.getPortfolio()));

        DynamicOptionParams params;
        params.initial_target_dte_years = 0.25;
        params.moneyness_ratio = 1.0;
        params.risk_free_rate = 0.05;
        params.rebalance_threshold = 0.01;
        params.vol_window_size = 10;

        strategy->addAssetConfig(aapl, params);
        strategy->addAssetConfig(msft, params);

        engine.setStrategy(std::move(strategy));

        BacktestResults results = engine.run();

        EXPECT_EQ(results.initial_cash, 1000000.0);
        EXPECT_GT(results.total_events_processed, 0);
    }

}
