#include "core/EventBus.h"

namespace kisansetu::core {

void EventBus::registerAgent(contracts::AgentId id, Agent* agent) {
    agents_[id] = agent;
}

void EventBus::unregisterAgent(const contracts::AgentId& id) {
    agents_.erase(id);
}

void EventBus::send(contracts::Envelope envelope) {
    contracts::SimTime scheduledTime = envelope.sentAt() + contracts::SimTime(latencyTicks_);
    sendAt(std::move(envelope), scheduledTime);
}

void EventBus::sendAt(contracts::Envelope envelope, contracts::SimTime deliveryTime) {
    if (dropRate_ > 0.0 && rng_ != nullptr) {
        if (rng_->bernoulli(dropRate_)) {
            eventLog_.log(deliveryTime, "MESSAGE_DROPPED_PACKET_LOSS",
                          envelope.sender(), envelope.receiver(),
                          envelope.correlationId(),
                          "Dropped due to simulated loss (rate=" + std::to_string(dropRate_) + ")");
            return;
        }
    }

    eventLog_.log(deliveryTime, "MESSAGE_QUEUED",
                  envelope.sender(), envelope.receiver(),
                  envelope.correlationId(),
                  "Performative=" + toString(envelope.performative()) +
                  " DeliverAt=" + std::to_string(deliveryTime.ticks()) + "m");

    scheduler_.schedule(deliveryTime, std::move(envelope));
}

bool EventBus::dispatchNext() {
    if (scheduler_.empty()) {
        return false;
    }

    auto scheduled = scheduler_.pop();

    if (clock_ != nullptr && scheduled.time > clock_->now()) {
        clock_->advanceTo(scheduled.time);
    }

    auto it = agents_.find(scheduled.envelope.receiver());
    if (it != agents_.end() && it->second != nullptr) {
        eventLog_.log(scheduled.time, "MESSAGE_DELIVERED",
                      scheduled.envelope.sender(),
                      scheduled.envelope.receiver(),
                      scheduled.envelope.correlationId(),
                      "Delivered to mailbox of " + it->second->id().str());

        it->second->deliver(std::move(scheduled.envelope));
        it->second->processNextMessage();
    } else {
        eventLog_.log(scheduled.time, "MESSAGE_DROPPED_UNKNOWN_RECIPIENT",
                      scheduled.envelope.sender(),
                      scheduled.envelope.receiver(),
                      scheduled.envelope.correlationId(),
                      "Receiver not registered on EventBus");
    }

    return true;
}

size_t EventBus::dispatchUpTo(contracts::SimTime maxTime) {
    size_t count = 0;
    while (!scheduler_.empty()) {
        auto nextTime = scheduler_.nextEventTime();
        if (!nextTime.has_value() || *nextTime > maxTime) {
            break;
        }
        dispatchNext();
        ++count;
    }
    return count;
}

size_t EventBus::dispatchAll() {
    size_t count = 0;
    while (dispatchNext()) {
        ++count;
    }
    return count;
}

size_t EventBus::registeredAgentCount() const {
    return agents_.size();
}

bool EventBus::hasAgent(const contracts::AgentId& id) const {
    return agents_.find(id) != agents_.end();
}

} // namespace kisansetu::core
