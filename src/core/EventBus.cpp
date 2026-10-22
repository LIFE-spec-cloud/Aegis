#include "core/EventBus.h"

namespace kisansetu::core {

void EventBus::registerAgent(contracts::AgentId id, Agent* agent) {
    agents_[id] = agent;
}

void EventBus::unregisterAgent(const contracts::AgentId& id) {
    agents_.erase(id);
}

void EventBus::send(contracts::Envelope envelope) {
    eventLog_.log(envelope.sentAt(), "MESSAGE_QUEUED", envelope.sender(), envelope.receiver(),
                  envelope.correlationId(), "Performative: " + toString(envelope.performative()));
    scheduler_.schedule(envelope.sentAt(), std::move(envelope));
}

void EventBus::dispatchNext() {
    if (scheduler_.empty()) {
        return;
    }
    auto scheduled = scheduler_.pop();
    auto it = agents_.find(scheduled.envelope.receiver());
    if (it != agents_.end() && it->second != nullptr) {
        eventLog_.log(scheduled.time, "MESSAGE_DELIVERED", scheduled.envelope.sender(),
                      scheduled.envelope.receiver(), scheduled.envelope.correlationId(),
                      "Delivered to " + it->second->id().str());
        it->second->onMessage(scheduled.envelope);
    }
}

size_t EventBus::registeredAgentCount() const {
    return agents_.size();
}

} // namespace kisansetu::core
