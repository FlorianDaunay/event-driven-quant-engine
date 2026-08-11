#ifndef IMPLIED_VOL_HPP
#define IMPLIED_VOL_HPP

#include "pricing/Option.hpp"

namespace quant
{

    enum class VolSolverMethod
    {
        NewtonRaphson,
        Bisection
    };

    class ImpliedVol
    {
    public:
        static double solve(const Option &option, const MarketData &market, double target_price,
                            VolSolverMethod method = VolSolverMethod::NewtonRaphson,
                            double tol = 1e-6, std::size_t max_iter = 100);

        static double solveNewtonRaphson(const Option &option, const MarketData &market, double target_price,
                                         double tol = 1e-6, std::size_t max_iter = 100);

        static double solveBisection(const Option &option, const MarketData &market, double target_price,
                                     double low_vol = 1e-4, double high_vol = 5.0,
                                     double tol = 1e-6, std::size_t max_iter = 100);
    };

}

#endif