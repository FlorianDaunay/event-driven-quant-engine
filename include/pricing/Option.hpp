#ifndef OPTION_HPP
#define OPTION_HPP

#include <cmath>
#include <algorithm>

namespace quant
{

    enum class OptionType
    {
        Call,
        Put
    };

    enum class ExerciseType
    {
        European,
        American
    };

    struct MarketData
    {
        double spot;
        double rate;
        double dividend;
        double volatility;

        MarketData(double s, double r, double q, double v)
            : spot(s), rate(r), dividend(q), volatility(v) {}
    };

    class Payoff
    {
    public:
        Payoff(OptionType type, double strike)
            : type_(type), strike_(strike) {}

        inline double operator()(double spot) const
        {
            switch (type_)
            {
            case OptionType::Call:
                return std::max(spot - strike_, 0.0);
            case OptionType::Put:
                return std::max(strike_ - spot, 0.0);
            default:
                return 0.0;
            }
        }

        OptionType getType() const { return type_; }
        double getStrike() const { return strike_; }

    private:
        OptionType type_;
        double strike_;
    };

    class Option
    {
    public:
        Option(OptionType type, ExerciseType exercise, double strike, double expiry)
            : payoff_(type, strike), exerciseType_(exercise), expiry_(expiry) {}

        OptionType getType() const { return payoff_.getType(); }
        ExerciseType getExerciseType() const { return exerciseType_; }
        double getStrike() const { return payoff_.getStrike(); }
        double getExpiry() const { return expiry_; }

        double payoff(double spot) const
        {
            return payoff_(spot);
        }

        bool isEuropean() const { return exerciseType_ == ExerciseType::European; }
        bool isAmerican() const { return exerciseType_ == ExerciseType::American; }

    private:
        Payoff payoff_;
        ExerciseType exerciseType_;
        double expiry_;
    };

}

#endif