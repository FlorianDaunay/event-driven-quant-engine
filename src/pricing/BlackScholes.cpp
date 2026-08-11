#include "pricing/BlackScholes.hpp"
#include <cmath>
#include <stdexcept>

namespace quant
{

    double BlackScholes::cdf(double x)
    {
        static const double inv_sqrt_2 = 0.70710678118654752440084436210485;
        return 0.5 * std::erfc(-x * inv_sqrt_2);
    }

    double BlackScholes::pdf(double x)
    {
        static const double inv_sqrt_2pi = 0.39894228040143267793994605993438;
        return inv_sqrt_2pi * std::exp(-0.5 * x * x);
    }

    double BlackScholes::d1(double S, double K, double r, double q, double v, double T)
    {
        return (std::log(S / K) + (r - q + 0.5 * v * v) * T) / (v * std::sqrt(T));
    }

    double BlackScholes::d2(double d1_val, double v, double T)
    {
        return d1_val - v * std::sqrt(T);
    }

    double BlackScholes::price(const Option &option, const MarketData &market)
    {
        if (!option.isEuropean())
        {
            throw std::invalid_argument("BlackScholes model only supports European options.");
        }

        double S = market.spot;
        double K = option.getStrike();
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;
        double T = option.getExpiry();

        if (T <= 0.0)
        {
            return option.payoff(S);
        }

        double d1_val = d1(S, K, r, q, v, T);
        double d2_val = d2(d1_val, v, T);

        if (option.getType() == OptionType::Call)
        {
            return S * std::exp(-q * T) * cdf(d1_val) - K * std::exp(-r * T) * cdf(d2_val);
        }
        else
        {
            return K * std::exp(-r * T) * cdf(-d2_val) - S * std::exp(-q * T) * cdf(-d1_val);
        }
    }

    double BlackScholes::delta(const Option &option, const MarketData &market)
    {
        double S = market.spot;
        double K = option.getStrike();
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;
        double T = option.getExpiry();

        double d1_val = d1(S, K, r, q, v, T);

        if (option.getType() == OptionType::Call)
        {
            return std::exp(-q * T) * cdf(d1_val);
        }
        else
        {
            return -std::exp(-q * T) * cdf(-d1_val);
        }
    }

    double BlackScholes::gamma(const Option &option, const MarketData &market)
    {
        double S = market.spot;
        double K = option.getStrike();
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;
        double T = option.getExpiry();

        double d1_val = d1(S, K, r, q, v, T);

        return std::exp(-q * T) * pdf(d1_val) / (S * v * std::sqrt(T));
    }

    double BlackScholes::vega(const Option &option, const MarketData &market)
    {
        double S = market.spot;
        double K = option.getStrike();
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;
        double T = option.getExpiry();

        double d1_val = d1(S, K, r, q, v, T);

        return S * std::exp(-q * T) * pdf(d1_val) * std::sqrt(T);
    }

    double BlackScholes::theta(const Option &option, const MarketData &market)
    {
        double S = market.spot;
        double K = option.getStrike();
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;
        double T = option.getExpiry();

        double d1_val = d1(S, K, r, q, v, T);
        double d2_val = d2(d1_val, v, T);

        double term1 = -(S * std::exp(-q * T) * pdf(d1_val) * v) / (2.0 * std::sqrt(T));

        if (option.getType() == OptionType::Call)
        {
            return term1 + q * S * std::exp(-q * T) * cdf(d1_val) - r * K * std::exp(-r * T) * cdf(d2_val);
        }
        else
        {
            return term1 - q * S * std::exp(-q * T) * cdf(-d1_val) + r * K * std::exp(-r * T) * cdf(-d2_val);
        }
    }

    double BlackScholes::rho(const Option &option, const MarketData &market)
    {
        double S = market.spot;
        double K = option.getStrike();
        double r = market.rate;
        double q = market.dividend;
        double v = market.volatility;
        double T = option.getExpiry();

        double d1_val = d1(S, K, r, q, v, T);
        double d2_val = d2(d1_val, v, T);

        if (option.getType() == OptionType::Call)
        {
            return K * T * std::exp(-r * T) * cdf(d2_val);
        }
        else
        {
            return -K * T * std::exp(-r * T) * cdf(-d2_val);
        }
    }

    Greeks BlackScholes::greeks(const Option &option, const MarketData &market)
    {
        return {
            delta(option, market),
            gamma(option, market),
            vega(option, market),
            theta(option, market),
            rho(option, market)};
    }

    Greeks BlackScholes::greeksFiniteDifference(const Option &option, const MarketData &market, double h)
    {
        double base_price = price(option, market);

        MarketData m_spot_plus = market;
        m_spot_plus.spot += h;
        MarketData m_spot_minus = market;
        m_spot_minus.spot -= h;

        double p_spot_plus = price(option, m_spot_plus);
        double p_spot_minus = price(option, m_spot_minus);

        double delta_fd = (p_spot_plus - p_spot_minus) / (2.0 * h);
        double gamma_fd = (p_spot_plus - 2.0 * base_price + p_spot_minus) / (h * h);

        MarketData m_vol_plus = market;
        m_vol_plus.volatility += h;
        MarketData m_vol_minus = market;
        m_vol_minus.volatility -= h;

        double vega_fd = (price(option, m_vol_plus) - price(option, m_vol_minus)) / (2.0 * h);

        MarketData m_rate_plus = market;
        m_rate_plus.rate += h;
        MarketData m_rate_minus = market;
        m_rate_minus.rate -= h;

        double rho_fd = (price(option, m_rate_plus) - price(option, m_rate_minus)) / (2.0 * h);

        double dt = h;
        if (option.getExpiry() <= dt)
        {
            dt = option.getExpiry() * 0.5;
        }

        Option option_t_minus = Option(option.getType(), option.getExerciseType(), option.getStrike(), option.getExpiry() - dt);
        double p_t_minus = price(option_t_minus, market);
        double theta_fd = (p_t_minus - base_price) / dt;

        return {delta_fd, gamma_fd, vega_fd, theta_fd, rho_fd};
    }

}