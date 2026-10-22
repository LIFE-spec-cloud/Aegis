#pragma once

#include "contracts/Types.h"
#include "contracts/Envelope.h"

namespace kisansetu::core {

/**
 * @brief Abstract base class for autonomous agents in KisanSetu.
 *
 * Each agent owns its private state. No agent holds direct pointers
 * or references to another agent.
 */
class Agent {
public:
    explicit Agent(contracts::AgentId id) : id_(std::move(id)) {}
    virtual ~Agent() = default;

    [[nodiscard]] const contracts::AgentId& id() const noexcept { return id_; }

    virtual void onMessage(const contracts::Envelope& envelope) = 0;

private:
    contracts::AgentId id_;
};

} // namespace kisansetu::core
