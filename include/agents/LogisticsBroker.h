#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include "contracts/IReservationPorts.h"
#include "contracts/IRecoveryContext.h"
#include "contracts/EventLog.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kisansetu::agents {

/**
 * @brief Mediator Agent (LogisticsBroker).
 *
 * Runs Contract Net Protocol (CNP), two-phase commit, escrow, and routes recovery.
 * Implements ITruckLockPort, IEscrowPort, and IRecoveryContext.
 *
 * Enforces architectural boundary: No agent includes another agent's header.
 */
class LogisticsBroker : public core::Agent,
                        public contracts::ITruckLockPort,
                        public contracts::IEscrowPort,
                        public contracts::IRecoveryContext {
public:
    LogisticsBroker(contracts::AgentId id, contracts::EventLog& eventLog)
        : core::Agent(std::move(id)),
          eventLog_(eventLog) {}

    void onMessage(const contracts::Envelope& envelope) override;

    // ITruckLockPort
    [[nodiscard]] std::optional<contracts::TruckLockHandle> lockTruck(
        const contracts::AgentId& truckId,
        const contracts::ContractId& contractId) override;

    bool releaseTruck(contracts::TruckLockHandle& handle) override;

    bool rebindTruck(
        contracts::TruckLockHandle& handle,
        const contracts::AgentId& newTruckId) override;

    // IEscrowPort
    [[nodiscard]] std::optional<contracts::EscrowHandle> holdDeposit(
        const contracts::AgentId& farmerId,
        const contracts::ContractId& contractId,
        contracts::Money amount) override;

    bool refundDeposit(contracts::EscrowHandle& handle) override;

    bool transferDeposit(
        contracts::EscrowHandle& handle,
        const contracts::AgentId& recipientId,
        contracts::Money transferAmount) override;

    // IRecoveryContext
    [[nodiscard]] std::vector<contracts::AgentId> findIdleTrucks(
        const contracts::NodeId& location,
        contracts::Quintals capacity) const override;

    [[nodiscard]] std::vector<contracts::MandiCandidate> rankMandis(
        const contracts::CropBatchInfo& batch,
        const std::vector<contracts::AgentId>& exclude) const override;

    [[nodiscard]] std::vector<contracts::AgentId> findProcessors(
        const contracts::NodeId& location,
        double radiusKm) const override;

    [[nodiscard]] contracts::RouteInfo routeTo(
        const contracts::NodeId& from,
        const contracts::NodeId& to,
        contracts::RouteStrategyType strategy) const override;

    [[nodiscard]] double predictQuality(
        const contracts::CropBatchInfo& batch,
        const contracts::RouteInfo& route,
        double ambientTempC) const override;

private:
    contracts::EventLog& eventLog_;
    uint64_t nextTruckLockId_{0};
    uint64_t nextEscrowId_{0};
    std::unordered_set<contracts::AgentId> lockedTrucks_;
    std::unordered_map<contracts::ContractId, contracts::Money> escrowLedger_;
};

} // namespace kisansetu::agents
