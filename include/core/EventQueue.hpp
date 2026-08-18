#ifndef EVENTQUEUE_HPP
#define EVENTQUEUE_HPP

#include <queue>
#include <memory>
#include "core/Event.hpp"

namespace quant
{

    class EventQueue
    {
    public:
        EventQueue() = default;
        ~EventQueue() = default;

        EventQueue(const EventQueue &) = delete;
        EventQueue &operator=(const EventQueue &) = delete;

        EventQueue(EventQueue &&) = default;
        EventQueue &operator=(EventQueue &&) = default;

        void push(std::shared_ptr<Event> event);
        std::shared_ptr<Event> pop();

        bool empty() const;
        size_t size() const;
        void clear();

    private:
        std::queue<std::shared_ptr<Event>> queue_;
    };

}

#endif