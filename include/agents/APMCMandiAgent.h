#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include "contracts/IReservationPorts.h"
#include "contracts/EventLog.h"
#include "core/EventBus.h"
#include "strategy/IPricingStrategy.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace kisansetu::agents {

/**
 * @brief Autonomous APMC Mandi Agent.
 *
 * Sole authority and owner of gate slots. Implements ISlotPort.
 * Private state: order book, yard congestion, hourly gate slots, pricing strategy.
 */
class APMCMandiAgent : public core::Agent, public contracts::ISlotPort {
public:
    APMCMandiAgent(contracts::AgentId id,
                   contracts::NodeId location,
                   contracts::Money basePrice,
                   size_t gateQuotaPerHour);

    void setDependencies(core::EventBus* bus,
                         contracts::EventLog* log,
                         std::unique_ptr<strategy::IPricingStrategy> pricingStrategy);

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
    [[nodiscard]] size_t getOccupancy(contracts::SimTime slotTime) const;

    [[nodiscard]] contracts::Money quoteCurrentPrice(contracts::CommodityType crop, contracts::SimTime slotTime) const;
    contracts::Money clearCallAuction(contracts::SimTime windowTime, contracts::CommodityType crop);

    void setBasePrice(contracts::Money newPrice) noexcept { basePrice_ = newPrice; }

private:
    contracts::NodeId location_;
    contracts::Money basePrice_{0};
    size_t gateQuotaPerHour_{4};
    uint64_t nextSlotReservationId_{0};

    core::EventBus* eventBus_{nullptr};
    contracts::EventLog* eventLog_{nullptr};
    std::unique_ptr<strategy::IPricingStrategy> pricingStrategy_;

    std::unordered_map<int64_t, std::vector<contracts::ContractId>> hourlySlots_;
    std::vector<contracts::ProposePayload> orderBook_;
};

} // namespace kisansetu::agents
