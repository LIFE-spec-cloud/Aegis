#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"

namespace kisansetu::agents {

/**
 * @brief Autonomous Food Processing Plant Agent.
 *
 * Holds private state: intake capacity, price floor, processing lines.
 * Bids in distress auctions when produce falls below mandi threshold (0.60) but >= 0.40.
 */
class ProcessorAgent : public core::Agent {
public:
    ProcessorAgent(contracts::AgentId id,
                   contracts::NodeId location,
                   contracts::Quintals intakeCapacity,
                   contracts::Money priceFloor)
        : core::Agent(std::move(id)),
          location_(std::move(location)),
          intakeCapacity_(intakeCapacity),
          priceFloor_(priceFloor) {}

    void onMessage(const contracts::Envelope& envelope) override;

    [[nodiscard]] const contracts::NodeId& location() const noexcept { return location_; }
    [[nodiscard]] contracts::Quintals intakeCapacity() const noexcept { return intakeCapacity_; }
    [[nodiscard]] contracts::Money priceFloor() const noexcept { return priceFloor_; }

private:
    contracts::NodeId location_;
    contracts::Quintals intakeCapacity_{0.0};
    contracts::Money priceFloor_{0};
};

} // namespace kisansetu::agents
