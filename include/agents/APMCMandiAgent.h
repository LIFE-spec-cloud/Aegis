#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include "contracts/IReservationPorts.h"
#include <unordered_map>
#include <vector>

namespace kisansetu::agents {

/**
 * @brief Autonomous APMC Mandi Agent.
 *
 * Sole authority and owner of gate slots. Implements ISlotPort.
 * Private state: order book, buyer reserves, yard congestion, gate slots.
 */
class APMCMandiAgent : public core::Agent, public contracts::ISlotPort {
public:
    APMCMandiAgent(contracts::AgentId id,
                   contracts::NodeId location,
                   contracts::Money basePrice,
                   size_t gateQuotaPerHour)
        : core::Agent(std::move(id)),
          location_(std::move(location)),
          basePrice_(basePrice),
          gateQuotaPerHour_(gateQuotaPerHour) {}

    void onMessage(const contracts::Envelope& envelope) override;

    // ISlotPort implementation
    [[nodiscard]] std::optional<contracts::SlotReservationHandle> reserveSlot(
        const contracts::AgentId& mandiId,
        const contracts::ContractId& contractId,
        contracts::SimTime slotTime) override;

    bool cancelSlot(contracts::SlotReservationHandle& handle) override;

    [[nodiscard]] const contracts::NodeId& location() const noexcept { return location_; }
    [[nodiscard]] contracts::Money basePrice() const noexcept { return basePrice_; }
    [[nodiscard]] size_t gateQuotaPerHour() const noexcept { return gateQuotaPerHour_; }

private:
    contracts::NodeId location_;
    contracts::Money basePrice_{0};
    size_t gateQuotaPerHour_{4};
    uint64_t nextSlotReservationId_{0};
    std::unordered_map<int64_t, std::vector<contracts::ContractId>> hourlySlots_;
};

} // namespace kisansetu::agents
