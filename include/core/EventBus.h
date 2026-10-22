#pragma once

#include "contracts/Envelope.h"
#include "contracts/EventLog.h"
#include "core/Agent.h"
#include "core/Scheduler.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace kisansetu::core {

/**
 * @brief EventBus routes messages between agents deterministically through mailboxes and scheduler.
 */
class EventBus {
public:
    explicit EventBus(Scheduler& scheduler, contracts::EventLog& eventLog)
        : scheduler_(scheduler), eventLog_(eventLog) {}

    void registerAgent(contracts::AgentId id, Agent* agent);
    void unregisterAgent(const contracts::AgentId& id);

    void send(contracts::Envelope envelope);
    void dispatchNext();

    [[nodiscard]] size_t registeredAgentCount() const;

private:
    Scheduler& scheduler_;
    contracts::EventLog& eventLog_;
    std::unordered_map<contracts::AgentId, Agent*> agents_;
};

} // namespace kisansetu::core
