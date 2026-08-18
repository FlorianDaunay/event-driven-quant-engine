#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <cstdint>

namespace quant
{

    enum class SignalType
    {
        Buy,
        Sell,
        Hold,
        Liquidate
    };

    enum class OrderType
    {
        Market,
        Limit
    };

    enum class OrderSide
    {
        Buy,
        Sell
    };

    enum class SecurityType
    {
        Equity,
        Option
    };

    struct Symbol
    {
        std::string ticker;
        SecurityType type;

        Symbol() : ticker(""), type(SecurityType::Equity) {}

        Symbol(const std::string &t, SecurityType sec_type = SecurityType::Equity)
            : ticker(t), type(sec_type) {}

        Symbol(const char *t, SecurityType sec_type = SecurityType::Equity)
            : ticker(t), type(sec_type) {}

        bool operator==(const Symbol &other) const
        {
            return ticker == other.ticker && type == other.type;
        }

        bool operator<(const Symbol &other) const
        {
            if (ticker != other.ticker)
                return ticker < other.ticker;
            return type < other.type;
        }
    };

    struct TradeBar
    {
        uint64_t timestamp;
        double open;
        double high;
        double low;
        double close;
        double volume;
    };

    struct Fill
    {
        uint64_t timestamp;
        Symbol symbol;
        OrderSide side;
        double quantity;
        double price;
        double commission;
    };

}

#endif