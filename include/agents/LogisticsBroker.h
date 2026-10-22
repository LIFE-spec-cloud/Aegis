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
#include <functional>

namespace kisansetu::core { class EventBus; }

namespace kisansetu::agents {

/**
 * @brief Tracks a single contract negotiation session through the CNP lifecycle.
 */
struct ContractSession {
    contracts::ContractId contractId;
    contracts::AgentId farmerId;
    contracts::CommodityType crop;
    contracts::Quintals volume;
    contracts::NodeId origin;
    contracts::SimTime readyTime;
    contracts::SimTime deadline;
    contracts::ContractState state{contracts::ContractState::CREATED};

    std::vector<contracts::ProposePayload> transporterProposals;
    std::vector<contracts::ProposePayload> mandiProposals;

    // Awarded resources (populated after two-phase commit)
    std::optional<contracts::SlotReservationHandle> slotHandle;
    std::optional<contracts::TruckLockHandle> truckHandle;
    std::optional<contracts::EscrowHandle> escrowHandle;

    contracts::AgentId awardedTransporterId;
    contracts::AgentId awardedMandiId;
};

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

    void setEventBus(core::EventBus* bus) { eventBus_ = bus; }

    /// Register agent roles for CFP broadcast targeting
    void registerTransporter(const contracts::AgentId& id);
    void registerMandi(const contracts::AgentId& id, const contracts::NodeId& location);
    void registerFarmer(const contracts::AgentId& id);
    void registerProcessor(const contracts::AgentId& id);

    /// Slot port injection (Mandi-owned slot ports, keyed by AgentId)
    using SlotReserveFunc = std::function<std::optional<contracts::SlotReservationHandle>(
        const contracts::AgentId&, const contracts::ContractId&, contracts::SimTime)>;
    using SlotCancelFunc = std::function<bool(contracts::SlotReservationHandle&)>;
    void registerSlotPort(const contracts::AgentId& mandiId, SlotReserveFunc reserve, SlotCancelFunc cancel);

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

    // Session accessors (for testing)
    [[nodiscard]] const ContractSession* findSession(contracts::ContractId id) const;
    [[nodiscard]] size_t activeSessionCount() const { return sessions_.size(); }
    [[nodiscard]] uint64_t nextContractId() const { return nextContractId_; }

private:
    contracts::EventLog& eventLog_;
    core::EventBus* eventBus_{nullptr};
    uint64_t nextContractId_{1};
    uint64_t nextTruckLockId_{0};
    uint64_t nextEscrowId_{0};
    std::unordered_set<contracts::AgentId> lockedTrucks_;
    std::unordered_map<contracts::ContractId, contracts::Money> escrowLedger_;

    // Agent registries
    std::vector<contracts::AgentId> registeredTransporters_;
    std::vector<contracts::AgentId> registeredFarmers_;
    std::vector<contracts::AgentId> registeredProcessors_;
    struct MandiInfo { contracts::AgentId id; contracts::NodeId location; };
    std::vector<MandiInfo> registeredMandis_;

    // Slot port callbacks (Mandi-owned, keyed by mandiId)
    struct SlotPort { SlotReserveFunc reserve; SlotCancelFunc cancel; };
    std::unordered_map<contracts::AgentId, SlotPort> slotPorts_;

    // Active contract sessions
    std::unordered_map<contracts::ContractId, ContractSession> sessions_;

    // Internal CNP methods
    void handleTaskAnnouncement(const contracts::Envelope& envelope);
    void handleProposal(const contracts::Envelope& envelope);
    void handleFarmerAccept(const contracts::Envelope& envelope);
    void handleFarmerReject(const contracts::Envelope& envelope);
    void handleFailure(const contracts::Envelope& envelope);

    void broadcastCfp(ContractSession& session, contracts::SimTime sentAt);
    bool executeCommit(ContractSession& session, const contracts::AcceptPayload& accept,
                       contracts::SimTime sentAt);
    void rollbackSession(ContractSession& session, contracts::SimTime sentAt,
                         const std::string& reason);
};

} // namespace kisansetu::agents
