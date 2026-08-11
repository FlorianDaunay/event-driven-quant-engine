#include "pricing/BinomialTree.hpp"
#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace quant
{

    void BinomialTree::buildParams(const MarketData &market, double dt, double &u, double &d, double &p, double &discount)
    {
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;

        u = std::exp(v * std::sqrt(dt));
        d = 1.0 / u;

        double growth = std::exp((r - q) * dt);
        p = (growth - d) / (u - d);
        discount = std::exp(-r * dt);
    }

    double BinomialTree::price(const Option &option, const MarketData &market, std::size_t steps)
    {
        if (steps == 0)
        {
            throw std::invalid_argument("Number of steps must be greater than zero.");
        }

        double T = option.getExpiry();
        if (T <= 0.0)
        {
            return option.payoff(market.spot);
        }

        double dt = T / static_cast<double>(steps);
        double u, d, p, discount;
        buildParams(market, dt, u, d, p, discount);

        std::vector<double> option_values(steps + 1);
        std::vector<double> spot_values(steps + 1);

        for (std::size_t j = 0; j <= steps; ++j)
        {
            spot_values[j] = market.spot * std::pow(u, static_cast<double>(steps - j)) * std::pow(d, static_cast<double>(j));
            option_values[j] = option.payoff(spot_values[j]);
        }

        bool is_american = option.isAmerican();

        for (int i = static_cast<int>(steps) - 1; i >= 0; --i)
        {
            for (std::size_t j = 0; j <= static_cast<std::size_t>(i); ++j)
            {
                double continuation = discount * (p * option_values[j] + (1.0 - p) * option_values[j + 1]);

                if (is_american)
                {
                    double spot_node = market.spot * std::pow(u, static_cast<double>(i - static_cast<int>(j))) * std::pow(d, static_cast<double>(j));
                    double exercise = option.payoff(spot_node);
                    option_values[j] = std::max(continuation, exercise);
                }
                else
                {
                    option_values[j] = continuation;
                }
            }
        }

        return option_values[0];
    }

    Greeks BinomialTree::greeks(const Option &option, const MarketData &market, std::size_t steps, double h)
    {
        double base_price = price(option, market, steps);

        MarketData m_spot_plus = market;
        m_spot_plus.spot += h;
        MarketData m_spot_minus = market;
        m_spot_minus.spot -= h;

        double p_spot_plus = price(option, m_spot_plus, steps);
        double p_spot_minus = price(option, m_spot_minus, steps);

        double delta_fd = (p_spot_plus - p_spot_minus) / (2.0 * h);
        double gamma_fd = (p_spot_plus - 2.0 * base_price + p_spot_minus) / (h * h);

        MarketData m_vol_plus = market;
        m_vol_plus.volatility += h;
        MarketData m_vol_minus = market;
        m_vol_minus.volatility -= h;

        double vega_fd = (price(option, m_vol_plus, steps) - price(option, m_vol_minus, steps)) / (2.0 * h);

        MarketData m_rate_plus = market;
        m_rate_plus.rate += h;
        MarketData m_rate_minus = market;
        m_rate_minus.rate -= h;

        double rho_fd = (price(option, m_rate_plus, steps) - price(option, m_rate_minus, steps)) / (2.0 * h);

        double dt = h;
        if (option.getExpiry() <= dt)
        {
            dt = option.getExpiry() * 0.5;
        }

        Option option_t_minus = Option(option.getType(), option.getExerciseType(), option.getStrike(), option.getExpiry() - dt);
        double p_t_minus = price(option_t_minus, market, steps);
        double theta_fd = -(p_t_minus - base_price) / dt;

        return {delta_fd, gamma_fd, vega_fd, theta_fd, rho_fd};
    }

}