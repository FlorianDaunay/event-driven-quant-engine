#include "core/Event.hpp"

namespace quant
{

    MarketEvent::MarketEvent(uint64_t timestamp, Symbol symbol, TradeBar bar)
        : timestamp_(timestamp), symbol_(symbol), bar_(bar) {}

    EventType MarketEvent::getType() const
    {
        return EventType::Market;
    }

    uint64_t MarketEvent::getTimestamp() const
    {
        return timestamp_;
    }

    const Symbol &MarketEvent::getSymbol() const
    {
        return symbol_;
    }

    const TradeBar &MarketEvent::getBar() const
    {
        return bar_;
    }

    SignalEvent::SignalEvent(uint64_t timestamp, Symbol symbol, SignalType signal_type, double strength)
        : timestamp_(timestamp), symbol_(symbol), signal_type_(signal_type), strength_(strength) {}

    EventType SignalEvent::getType() const
    {
        return EventType::Signal;
    }

    uint64_t SignalEvent::getTimestamp() const
    {
        return timestamp_;
    }

    const Symbol &SignalEvent::getSymbol() const
    {
        return symbol_;
    }

    SignalType SignalEvent::getSignalType() const
    {
        return signal_type_;
    }

    double SignalEvent::getStrength() const
    {
        return strength_;
    }

    OrderEvent::OrderEvent(uint64_t timestamp, Symbol symbol, OrderType order_type, OrderSide side, double quantity, double limit_price)
        : timestamp_(timestamp), symbol_(symbol), order_type_(order_type), side_(side), quantity_(quantity), limit_price_(limit_price) {}

    EventType OrderEvent::getType() const
    {
        return EventType::Order;
    }

    uint64_t OrderEvent::getTimestamp() const
    {
        return timestamp_;
    }

    const Symbol &OrderEvent::getSymbol() const
    {
        return symbol_;
    }

    OrderType OrderEvent::getOrderType() const
    {
        return order_type_;
    }

    OrderSide OrderEvent::getSide() const
    {
        return side_;
    }

    double OrderEvent::getQuantity() const
    {
        return quantity_;
    }

    double OrderEvent::getLimitPrice() const
    {
        return limit_price_;
    }

    FillEvent::FillEvent(uint64_t timestamp, Symbol symbol, OrderSide side, double quantity, double price, double commission)
        : timestamp_(timestamp), symbol_(symbol), side_(side), quantity_(quantity), price_(price), commission_(commission) {}

    EventType FillEvent::getType() const
    {
        return EventType::Fill;
    }

    uint64_t FillEvent::getTimestamp() const
    {
        return timestamp_;
    }

    const Symbol &FillEvent::getSymbol() const
    {
        return symbol_;
    }

    OrderSide FillEvent::getSide() const
    {
        return side_;
    }

    double FillEvent::getQuantity() const
    {
        return quantity_;
    }

    double FillEvent::getPrice() const
    {
        return price_;
    }

    double FillEvent::getCommission() const
    {
        return commission_;
    }

}