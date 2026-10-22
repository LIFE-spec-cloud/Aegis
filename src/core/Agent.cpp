#include "core/Agent.h"

namespace kisansetu::core {

Agent::Agent(contracts::AgentId id) : id_(std::move(id)) {}

void Agent::deliver(contracts::Envelope envelope) {
    std::lock_guard<std::mutex> lock(mailboxMutex_);
    mailbox_.push_back(std::move(envelope));
}

bool Agent::hasMessages() const {
    std::lock_guard<std::mutex> lock(mailboxMutex_);
    return !mailbox_.empty();
}

size_t Agent::mailboxSize() const {
    std::lock_guard<std::mutex> lock(mailboxMutex_);
    return mailbox_.size();
}

std::optional<contracts::Envelope> Agent::pollMessage() {
    std::lock_guard<std::mutex> lock(mailboxMutex_);
    if (mailbox_.empty()) {
        return std::nullopt;
    }
    contracts::Envelope front = std::move(mailbox_.front());
    mailbox_.pop_front();
    return front;
}

bool Agent::processNextMessage() {
    auto msg = pollMessage();
    if (!msg.has_value()) {
        return false;
    }
    onMessage(*msg);
    return true;
}

size_t Agent::processAllMessages() {
    size_t count = 0;
    while (processNextMessage()) {
        ++count;
    }
    return count;
}

} // namespace kisansetu::core
