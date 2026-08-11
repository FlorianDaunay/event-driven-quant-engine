#include "pricing/ImpliedVol.hpp"
#include "pricing/BlackScholes.hpp"
#include "pricing/BinomialTree.hpp"
#include <cmath>
#include <stdexcept>

namespace quant
{

    namespace
    {

        double calculatePrice(const Option &option, const MarketData &market)
        {
            if (option.isEuropean())
            {
                return BlackScholes::price(option, market);
            }
            else
            {
                return BinomialTree::price(option, market);
            }
        }

        double calculateVega(const Option &option, const MarketData &market)
        {
            if (option.isEuropean())
            {
                return BlackScholes::vega(option, market);
            }
            else
            {
                return BinomialTree::greeks(option, market).vega;
            }
        }

    }

    double ImpliedVol::solve(const Option &option, const MarketData &market, double target_price,
                             VolSolverMethod method, double tol, std::size_t max_iter)
    {
        if (method == VolSolverMethod::NewtonRaphson)
        {
            try
            {
                return solveNewtonRaphson(option, market, target_price, tol, max_iter);
            }
            catch (const std::runtime_error &)
            {
                return solveBisection(option, market, target_price, 1e-4, 5.0, tol, max_iter);
            }
        }
        return solveBisection(option, market, target_price, 1e-4, 5.0, tol, max_iter);
    }

    double ImpliedVol::solveNewtonRaphson(const Option &option, const MarketData &market, double target_price,
                                          double tol, std::size_t max_iter)
    {
        double S = market.spot;
        double K = option.getStrike();
        double T = option.getExpiry();

        double sigma = std::sqrt(2.0 * std::abs(std::log(S / K) + market.rate * T) / T);
        if (sigma < 1e-3 || std::isnan(sigma))
        {
            sigma = 0.20;
        }

        MarketData current_market = market;

        for (std::size_t i = 0; i < max_iter; ++i)
        {
            current_market.volatility = sigma;

            double price = calculatePrice(option, current_market);
            double diff = price - target_price;

            if (std::abs(diff) < tol)
            {
                return sigma;
            }

            double vega = calculateVega(option, current_market);

            if (std::abs(vega) < 1e-8)
            {
                break;
            }

            sigma -= diff / vega;

            if (sigma <= 0.0)
            {
                sigma = 0.5 * (sigma + diff / vega);
            }
        }

        throw std::runtime_error("Newton-Raphson failed to converge for Implied Volatility calculation.");
    }

    double ImpliedVol::solveBisection(const Option &option, const MarketData &market, double target_price,
                                      double low_vol, double high_vol, double tol, std::size_t max_iter)
    {
        MarketData m_low = market;
        m_low.volatility = low_vol;
        double price_low = calculatePrice(option, m_low);

        MarketData m_high = market;
        m_high.volatility = high_vol;
        double price_high = calculatePrice(option, m_high);

        if ((target_price - price_low) * (target_price - price_high) > 0.0)
        {
            throw std::invalid_argument("Target price is outside the boundary conditions for volatility range.");
        }

        double mid_vol = low_vol;

        for (std::size_t i = 0; i < max_iter; ++i)
        {
            mid_vol = 0.5 * (low_vol + high_vol);

            MarketData m_mid = market;
            m_mid.volatility = mid_vol;
            double price_mid = calculatePrice(option, m_mid);

            double diff = price_mid - target_price;

            if (std::abs(diff) < tol || 0.5 * (high_vol - low_vol) < tol)
            {
                return mid_vol;
            }

            if ((price_mid - target_price) * (price_low - target_price) < 0.0)
            {
                high_vol = mid_vol;
                price_high = price_mid;
            }
            else
            {
                low_vol = mid_vol;
                price_low = price_mid;
            }
        }

        return mid_vol;
    }

}