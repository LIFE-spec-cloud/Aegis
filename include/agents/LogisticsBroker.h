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
namespace kisansetu::domain { class RoadGraph; }

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
    void setRoadGraph(const domain::RoadGraph* graph) { roadGraph_ = graph; }

    /// Register agent roles for CFP broadcast targeting
    void registerTransporter(const contracts::AgentId& id,
                             contracts::Quintals capacity = contracts::Quintals(50.0),
                             const contracts::NodeId& location = contracts::NodeId("Depot_A"));
    void registerMandi(const contracts::AgentId& id,
                       const contracts::NodeId& location,
                       contracts::Money basePrice = contracts::Money::fromRupees(2000.0),
                       size_t capacity = 4);
    void registerFarmer(const contracts::AgentId& id);
    void registerProcessor(const contracts::AgentId& id,
                           const contracts::NodeId& location = contracts::NodeId("Plant_MotherDairy"));

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

    // Herd mitigation & Inbound tracking
    void addInboundTruck(const contracts::AgentId& mandiId);
    void removeInboundTruck(const contracts::AgentId& mandiId);
    [[nodiscard]] size_t getInboundTruckCount(const contracts::AgentId& mandiId) const;

    // Reroute hysteresis and bounded attempts
    [[nodiscard]] bool shouldReroute(
        contracts::ContractId cid,
        contracts::Money currentProjectedMargin,
        contracts::Money alternateMargin,
        contracts::Money deltaThreshold = contracts::Money::fromRupees(5000.0),
        size_t maxReroutes = 2) const;

    void recordReroute(
        contracts::ContractId cid,
        const contracts::AgentId& oldMandi,
        const contracts::AgentId& newMandi);

    [[nodiscard]] size_t getRerouteCount(contracts::ContractId cid) const;

    // Session accessors (for testing)
    [[nodiscard]] const ContractSession* findSession(contracts::ContractId id) const;
    [[nodiscard]] size_t activeSessionCount() const { return sessions_.size(); }
    [[nodiscard]] uint64_t nextContractId() const { return nextContractId_; }

private:
    contracts::EventLog& eventLog_;
    core::EventBus* eventBus_{nullptr};
    const domain::RoadGraph* roadGraph_{nullptr};
    uint64_t nextContractId_{1};
    uint64_t nextTruckLockId_{0};
    uint64_t nextEscrowId_{0};
    std::unordered_set<contracts::AgentId> lockedTrucks_;
    std::unordered_map<contracts::ContractId, contracts::Money> escrowLedger_;

    // Inbound traffic and reroute counts
    std::unordered_map<contracts::AgentId, size_t> inboundTrucksPerMandi_;
    std::unordered_map<contracts::ContractId, size_t> rerouteCounts_;

    // Agent registries
    struct TransporterInfo {
        contracts::AgentId id;
        contracts::Quintals capacity{50.0};
        contracts::NodeId location;
    };
    std::vector<TransporterInfo> registeredTransporters_;
    std::vector<contracts::AgentId> registeredFarmers_;

    struct ProcessorInfo {
        contracts::AgentId id;
        contracts::NodeId location;
    };
    std::vector<ProcessorInfo> registeredProcessors_;

    struct MandiInfo {
        contracts::AgentId id;
        contracts::NodeId location;
        contracts::Money basePrice{contracts::Money::fromRupees(2000.0)};
        size_t capacity{4};
    };
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
