#ifndef BINOMIAL_TREE_HPP
#define BINOMIAL_TREE_HPP

#include "pricing/Option.hpp"
#include "pricing/BlackScholes.hpp"

namespace quant
{

    class BinomialTree
    {
    public:
        static double price(const Option &option, const MarketData &market, std::size_t steps = 500);
        static Greeks greeks(const Option &option, const MarketData &market, std::size_t steps = 500, double h = 0.0001);

    private:
        static void buildParams(const MarketData &market, double dt, double &u, double &d, double &p, double &discount);
    };

}

#endif