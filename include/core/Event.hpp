#ifndef EVENT_HPP
#define EVENT_HPP

#include <string>
#include <memory>
#include <cstdint>
#include "core/Types.hpp"

namespace quant
{

    enum class EventType
    {
        Market,
        Signal,
        Order,
        Fill
    };

    class Event
    {
    public:
        virtual ~Event() = default;
        virtual EventType getType() const = 0;
        virtual uint64_t getTimestamp() const = 0;
    };

    class MarketEvent : public Event
    {
    public:
        MarketEvent(uint64_t timestamp, Symbol symbol, TradeBar bar);

        EventType getType() const override;
        uint64_t getTimestamp() const override;

        const Symbol &getSymbol() const;
        const TradeBar &getBar() const;

    private:
        uint64_t timestamp_;
        Symbol symbol_;
        TradeBar bar_;
    };

    class SignalEvent : public Event
    {
    public:
        SignalEvent(uint64_t timestamp, Symbol symbol, SignalType signal_type, double strength = 1.0);

        EventType getType() const override;
        uint64_t getTimestamp() const override;

        const Symbol &getSymbol() const;
        SignalType getSignalType() const;
        double getStrength() const;

    private:
        uint64_t timestamp_;
        Symbol symbol_;
        SignalType signal_type_;
        double strength_;
    };

    class OrderEvent : public Event
    {
    public:
        OrderEvent(uint64_t timestamp, Symbol symbol, OrderType order_type, OrderSide side, double quantity, double limit_price = 0.0);

        EventType getType() const override;
        uint64_t getTimestamp() const override;

        const Symbol &getSymbol() const;
        OrderType getOrderType() const;
        OrderSide getSide() const;
        double getQuantity() const;
        double getLimitPrice() const;

    private:
        uint64_t timestamp_;
        Symbol symbol_;
        OrderType order_type_;
        OrderSide side_;
        double quantity_;
        double limit_price_;
    };

    class FillEvent : public Event
    {
    public:
        FillEvent(uint64_t timestamp, Symbol symbol, OrderSide side, double quantity, double price, double commission);

        EventType getType() const override;
        uint64_t getTimestamp() const override;

        const Symbol &getSymbol() const;
        OrderSide getSide() const;
        double getQuantity() const;
        double getPrice() const;
        double getCommission() const;

    private:
        uint64_t timestamp_;
        Symbol symbol_;
        OrderSide side_;
        double quantity_;
        double price_;
        double commission_;
    };

}

#endif