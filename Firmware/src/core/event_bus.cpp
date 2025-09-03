#include "event_bus.h"

namespace CORE {
EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

uint32_t EventBus::subscribe(Event::Type type, EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    Subscription subscription;
    subscription.id = nextSubscriptionId_++;
    subscription.type = type;
    subscription.callback = callback;
    subscriptions_.push_back(subscription);
    
    return subscription.id;
}

bool EventBus::unsubscribe(uint32_t subscriptionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = subscriptions_.begin(); it != subscriptions_.end(); ++it) {
        if (it->id == subscriptionId) {
            subscriptions_.erase(it);
            return true;
        }
    }
    return false; 
}

void EventBus::publish(const Event& event) {
    std::vector<Subscription> subscriptionsCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptionsCopy = subscriptions_;
    }
    for (const auto& subscription : subscriptionsCopy) {
        if (subscription.type == event.getType()) {
            subscription.callback(event);
        }
    }
}

void EventBus::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.clear();
}

} // namespace CORE