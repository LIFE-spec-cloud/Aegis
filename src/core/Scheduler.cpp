#include "core/Scheduler.h"

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

Scheduler::ScheduledEvent Scheduler::pop() {
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
