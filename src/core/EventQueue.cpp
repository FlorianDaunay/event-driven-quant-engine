#include "core/EventQueue.hpp"
#include <stdexcept>

namespace quant
{

    void EventQueue::push(std::shared_ptr<Event> event)
    {
        if (event)
        {
            queue_.push(std::move(event));
        }
    }

    std::shared_ptr<Event> EventQueue::pop()
    {
        if (queue_.empty())
        {
            return nullptr;
        }

        auto event = queue_.front();
        queue_.pop();
        return event;
    }

    bool EventQueue::empty() const
    {
        return queue_.empty();
    }

    size_t EventQueue::size() const
    {
        return queue_.size();
    }

    void EventQueue::clear()
    {
        std::queue<std::shared_ptr<Event>> empty_queue;
        std::swap(queue_, empty_queue);
    }

}