#include <gtest/gtest.h>
#include <cmath>
#include "pricing/Option.hpp"
#include "pricing/BlackScholes.hpp"
#include "pricing/BinomialTree.hpp"
#include "pricing/ImpliedVol.hpp"

using namespace quant;

class PricingTest : public ::testing::Test
{
protected:
    double spot = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double dividend = 0.02;
    double volatility = 0.20;
    double expiry = 1.0;

    MarketData market{spot, rate, dividend, volatility};
};

TEST_F(PricingTest, BlackScholesCallPutParity)
{
    Option call(OptionType::Call, ExerciseType::European, strike, expiry);
    Option put(OptionType::Put, ExerciseType::European, strike, expiry);

    double call_price = BlackScholes::price(call, market);
    double put_price = BlackScholes::price(put, market);

    double lhs = call_price - put_price;
    double rhs = spot * std::exp(-dividend * expiry) - strike * std::exp(-rate * expiry);

    EXPECT_NEAR(lhs, rhs, 1e-6);
}

TEST_F(PricingTest, AnalyticalVsFiniteDifferenceGreeks)
{
    Option call(OptionType::Call, ExerciseType::European, strike, expiry);

    Greeks analytical = BlackScholes::greeks(call, market);
    Greeks fd = BlackScholes::greeksFiniteDifference(call, market, 0.0001);

    EXPECT_NEAR(analytical.delta, fd.delta, 1e-3);
    EXPECT_NEAR(analytical.gamma, fd.gamma, 1e-3);
    EXPECT_NEAR(analytical.vega, fd.vega, 1e-3);
    EXPECT_NEAR(analytical.theta, fd.theta, 1e-2);
    EXPECT_NEAR(analytical.rho, fd.rho, 1e-3);
}

TEST_F(PricingTest, BinomialTreeConvergenceToBlackScholes)
{
    Option call(OptionType::Call, ExerciseType::European, strike, expiry);

    double bs_price = BlackScholes::price(call, market);
    double tree_price = BinomialTree::price(call, market, 1000);

    EXPECT_NEAR(tree_price, bs_price, 1e-2);
}

TEST_F(PricingTest, AmericanOptionPremiumOverEuropean)
{
    Option eu_put(OptionType::Put, ExerciseType::European, strike, expiry);
    Option us_put(OptionType::Put, ExerciseType::American, strike, expiry);

    double eu_price = BinomialTree::price(eu_put, market, 500);
    double us_price = BinomialTree::price(us_put, market, 500);

    EXPECT_GE(us_price, eu_price);
}

TEST_F(PricingTest, ImpliedVolatilityInversionEuropean)
{
    Option call(OptionType::Call, ExerciseType::European, strike, expiry);

    double target_price = BlackScholes::price(call, market);

    MarketData m_unknown_vol = market;
    m_unknown_vol.volatility = 0.0;

    double solved_vol = ImpliedVol::solve(call, m_unknown_vol, target_price, VolSolverMethod::NewtonRaphson);

    EXPECT_NEAR(solved_vol, volatility, 1e-5);
}

TEST_F(PricingTest, ImpliedVolatilityInversionAmerican)
{
    Option us_put(OptionType::Put, ExerciseType::American, strike, expiry);

    double target_price = BinomialTree::price(us_put, market, 300);

    MarketData m_unknown_vol = market;
    m_unknown_vol.volatility = 0.0;

    double solved_vol = ImpliedVol::solve(us_put, m_unknown_vol, target_price, VolSolverMethod::Bisection);

    EXPECT_NEAR(solved_vol, volatility, 1e-3);
}