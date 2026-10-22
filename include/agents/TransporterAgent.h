#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"

namespace kisansetu::agents {

/**
 * @brief Autonomous Transporter Agent.
 *
 * Holds private state: cost/km, mechanical health, position, true ETA, state.
 */
class TransporterAgent : public core::Agent {
public:
    TransporterAgent(contracts::AgentId id,
                     contracts::NodeId baseLocation,
                     contracts::Quintals capacity,
                     contracts::Money costPerKm,
                     bool isRefrigerated)
        : core::Agent(std::move(id)),
          currentLocation_(std::move(baseLocation)),
          capacity_(capacity),
          costPerKm_(costPerKm),
          isRefrigerated_(isRefrigerated),
          state_(contracts::TruckState::IDLE) {}

    void onMessage(const contracts::Envelope& envelope) override;

    [[nodiscard]] const contracts::NodeId& currentLocation() const noexcept { return currentLocation_; }
    [[nodiscard]] contracts::Quintals capacity() const noexcept { return capacity_; }
    [[nodiscard]] contracts::Money costPerKm() const noexcept { return costPerKm_; }
    [[nodiscard]] bool isRefrigerated() const noexcept { return isRefrigerated_; }
    [[nodiscard]] contracts::TruckState state() const noexcept { return state_; }

private:
    contracts::NodeId currentLocation_;
    contracts::Quintals capacity_{0.0};
    contracts::Money costPerKm_{0};
    bool isRefrigerated_{false};
    contracts::TruckState state_{contracts::TruckState::IDLE};
};

} // namespace kisansetu::agents
