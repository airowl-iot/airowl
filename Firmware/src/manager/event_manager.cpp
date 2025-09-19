// event_bus.cpp
#include "event_manager.h"
#include <algorithm>

namespace CORE {

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

uint32_t EventBus::subscribe(Event::Type type, EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.push_back({nextSubscriptionId_++, type, callback});
    return nextSubscriptionId_ - 1;
}

bool EventBus::unsubscribe(uint32_t subscriptionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                             [subscriptionId](const Subscription& s){ return s.id == subscriptionId; });
    if (it != subscriptions_.end()) {
        subscriptions_.erase(it, subscriptions_.end());
        return true;
    }
    return false;
}

void EventBus::publish(const std::shared_ptr<const Event>& event) {
    std::vector<Subscription> copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy = subscriptions_;
    }
    for (auto& sub : copy) {
        if (sub.type == event->getType()) {
            sub.callback(event);
        }
    }
}

void EventBus::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.clear();
}

} // namespace CORE
