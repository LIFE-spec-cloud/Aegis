#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include "contracts/IOrderInputPort.h"
#include <optional>

namespace kisansetu::agents {

/**
 * @brief Autonomous Farmer Agent.
 *
 * Holds private state: reserve price, deposit budget, risk aversion, crop batches.
 * Communicates strictly via Envelope messages or IOrderInputPort domain commands.
 */
class FarmerAgent : public core::Agent {
public:
    FarmerAgent(contracts::AgentId id,
                contracts::Money reservePrice,
                contracts::Money depositBudget,
                double riskAversion)
        : core::Agent(std::move(id)),
          reservePrice_(reservePrice),
          depositBudget_(depositBudget),
          riskAversion_(riskAversion) {}

    void onMessage(const contracts::Envelope& envelope) override;

    void handleOrderCommand(const contracts::DomainCommand& command);

    [[nodiscard]] contracts::Money reservePrice() const noexcept { return reservePrice_; }
    [[nodiscard]] contracts::Money depositBudget() const noexcept { return depositBudget_; }
    [[nodiscard]] double riskAversion() const noexcept { return riskAversion_; }

private:
    contracts::Money reservePrice_{0};
    contracts::Money depositBudget_{0};
    double riskAversion_{0.1};
};

} // namespace kisansetu::agents
