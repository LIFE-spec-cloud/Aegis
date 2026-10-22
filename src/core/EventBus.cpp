#include "core/EventBus.h"
#include <future>
#include <unordered_set>
#include <vector>

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
    if (!parallelMode_) {
        while (dispatchNext()) {
            ++count;
        }
        return count;
    }

    // Parallel mode: batch delivery for current timestamp, then run agent mailboxes concurrently
    while (!scheduler_.empty()) {
        auto nextOpt = scheduler_.nextEventTime();
        if (!nextOpt.has_value()) {
            break;
        }
        auto currentTime = *nextOpt;
        if (clock_ != nullptr && currentTime > clock_->now()) {
            clock_->advanceTo(currentTime);
        }

        std::unordered_set<Agent*> agentsWithWork;
        while (!scheduler_.empty() && scheduler_.nextEventTime().value_or(currentTime + contracts::SimTime(1)) == currentTime) {
            auto scheduled = scheduler_.pop();
            auto it = agents_.find(scheduled.envelope.receiver());
            if (it != agents_.end() && it->second != nullptr) {
                eventLog_.log(scheduled.time, "MESSAGE_DELIVERED",
                              scheduled.envelope.sender(),
                              scheduled.envelope.receiver(),
                              scheduled.envelope.correlationId(),
                              "Delivered to mailbox of " + it->second->id().str());
                it->second->deliver(std::move(scheduled.envelope));
                agentsWithWork.insert(it->second);
            } else {
                eventLog_.log(scheduled.time, "MESSAGE_DROPPED_UNKNOWN_RECIPIENT",
                              scheduled.envelope.sender(),
                              scheduled.envelope.receiver(),
                              scheduled.envelope.correlationId(),
                              "Receiver not registered on EventBus");
            }
            ++count;
        }

        if (agentsWithWork.size() == 1) {
            (*agentsWithWork.begin())->processAllMessages();
        } else if (agentsWithWork.size() > 1) {
            std::vector<std::future<void>> futures;
            for (auto* agent : agentsWithWork) {
                futures.push_back(std::async(std::launch::async, [agent]() {
                    agent->processAllMessages();
                }));
            }
            for (auto& f : futures) {
                f.get();
            }
        }
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
