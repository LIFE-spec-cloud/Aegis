#pragma once

#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include <deque>
#include <mutex>
#include <optional>

namespace kisansetu::core {

/**
 * @brief Abstract base class for autonomous agents in KisanSetu.
 *
 * Each agent owns its private state and a dedicated mailbox.
 * No agent holds direct pointers or references to another agent.
 */
class Agent {
public:
    explicit Agent(contracts::AgentId id);
    virtual ~Agent() = default;

    // Non-copyable, non-movable polymorphic base
    Agent(const Agent&) = delete;
    Agent& operator=(const Agent&) = delete;
    Agent(Agent&&) = delete;
    Agent& operator=(Agent&&) = delete;

    [[nodiscard]] const contracts::AgentId& id() const noexcept { return id_; }

    /**
     * @brief Post an incoming envelope into this agent's mailbox.
     * Thread-safe for parallel mode.
     */
    void deliver(contracts::Envelope envelope);

    /**
     * @brief Check whether messages are pending in the mailbox.
     */
    [[nodiscard]] bool hasMessages() const;

    /**
     * @brief Number of messages currently in the mailbox.
     */
    [[nodiscard]] size_t mailboxSize() const;

    /**
     * @brief Retrieve and remove the next message from the mailbox, if available.
     */
    [[nodiscard]] std::optional<contracts::Envelope> pollMessage();

    /**
     * @brief Process the next pending message by invoking onMessage().
     * @return true if a message was processed, false if mailbox was empty.
     */
    bool processNextMessage();

    /**
     * @brief Drains all queued messages and processes them in FIFO order.
     * @return count of messages processed.
     */
    size_t processAllMessages();

    /**
     * @brief Pure virtual message handler implemented by derived agents.
     */
    virtual void onMessage(const contracts::Envelope& envelope) = 0;

private:
    contracts::AgentId id_;
    mutable std::mutex mailboxMutex_;
    std::deque<contracts::Envelope> mailbox_;
};

} // namespace kisansetu::core
