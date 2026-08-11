#ifndef BLACK_SCHOLES_HPP
#define BLACK_SCHOLES_HPP

#include "pricing/Option.hpp"

namespace quant
{

    struct Greeks
    {
        double delta;
        double gamma;
        double vega;
        double theta;
        double rho;
    };

    class BlackScholes
    {
    public:
        static double price(const Option &option, const MarketData &market);
        static Greeks greeks(const Option &option, const MarketData &market);

        static double delta(const Option &option, const MarketData &market);
        static double gamma(const Option &option, const MarketData &market);
        static double vega(const Option &option, const MarketData &market);
        static double theta(const Option &option, const MarketData &market);
        static double rho(const Option &option, const MarketData &market);

        static Greeks greeksFiniteDifference(const Option &option, const MarketData &market, double h = 0.0001);

    private:
        static double d1(double S, double K, double r, double q, double v, double T);
        static double d2(double d1_val, double v, double T);

        static double cdf(double x);
        static double pdf(double x);
    };

}

#endif