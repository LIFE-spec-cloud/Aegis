#include "core/Scheduler.h"
#include <stdexcept>

namespace kisansetu::core {

void Scheduler::schedule(contracts::SimTime time, contracts::Envelope envelope) {
    queue_.push(ScheduledEvent{
        .time = time,
        .sequenceNumber = ++nextSeq_,
        .envelope = std::move(envelope)
    });
}

bool Scheduler::empty() const noexcept {
    return queue_.empty();
}

size_t Scheduler::size() const noexcept {
    return queue_.size();
}

std::optional<contracts::SimTime> Scheduler::nextEventTime() const noexcept {
    if (queue_.empty()) {
        return std::nullopt;
    }
    return queue_.top().time;
}

Scheduler::ScheduledEvent Scheduler::pop() {
    if (queue_.empty()) {
        throw std::underflow_error("Scheduler::pop() called on empty queue");
    }
    auto top = std::move(const_cast<ScheduledEvent&>(queue_.top()));
    queue_.pop();
    return top;
}

void Scheduler::reset() {
    while (!queue_.empty()) {
        queue_.pop();
    }
    nextSeq_ = 0;
}

} // namespace kisansetu::core
