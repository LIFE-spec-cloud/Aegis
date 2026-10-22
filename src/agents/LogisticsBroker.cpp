#include "agents/LogisticsBroker.h"

namespace kisansetu::agents {

void LogisticsBroker::onMessage(const contracts::Envelope& /*envelope*/) {
    // Stub for A0; fully implemented in A3
}

// ITruckLockPort
std::optional<contracts::TruckLockHandle> LogisticsBroker::lockTruck(
    const contracts::AgentId& truckId,
    const contracts::ContractId& contractId) {
    if (lockedTrucks_.contains(truckId)) {
        return std::nullopt; // Already locked
    }
    lockedTrucks_.insert(truckId);
    eventLog_.log(contracts::SimTime(0), "TRUCK_LOCKED", id(), truckId, contractId, "Truck locked");
    return contracts::TruckLockHandle(truckId, contractId, ++nextTruckLockId_);
}

bool LogisticsBroker::releaseTruck(contracts::TruckLockHandle& handle) {
    if (!handle.isValid()) {
        return false;
    }
    eventLog_.log(contracts::SimTime(0), "TRUCK_RELEASED", id(), handle.truckId(), handle.contractId(), "Truck released");
    lockedTrucks_.erase(handle.truckId());
    handle.invalidate();
    return true;
}

bool LogisticsBroker::rebindTruck(
    contracts::TruckLockHandle& handle,
    const contracts::AgentId& newTruckId) {
    if (!handle.isValid()) {
        return false;
    }
    if (lockedTrucks_.contains(newTruckId)) {
        return false; // Replacement truck is already locked
    }
    eventLog_.log(contracts::SimTime(0), "TRUCK_REBOUND", handle.truckId(), newTruckId, handle.contractId(), "Truck rebound");
    lockedTrucks_.erase(handle.truckId());
    lockedTrucks_.insert(newTruckId);
    handle.rebind(newTruckId, ++nextTruckLockId_);
    return true;
}

// IEscrowPort
std::optional<contracts::EscrowHandle> LogisticsBroker::holdDeposit(
    const contracts::AgentId& farmerId,
    const contracts::ContractId& contractId,
    contracts::Money amount) {
    escrowLedger_[contractId] = amount;
    eventLog_.log(contracts::SimTime(0), "ESCROW_HELD", farmerId, id(), contractId, "Escrow deposit held: " + std::to_string(amount.toRupees()) + " INR");
    return contracts::EscrowHandle(farmerId, contractId, amount, ++nextEscrowId_);
}

bool LogisticsBroker::refundDeposit(contracts::EscrowHandle& handle) {
    if (!handle.isValid()) {
        return false;
    }
    eventLog_.log(contracts::SimTime(0), "ESCROW_REFUNDED", id(), handle.farmerId(), handle.contractId(), "Escrow deposit refunded");
    escrowLedger_.erase(handle.contractId());
    handle.invalidate();
    return true;
}

bool LogisticsBroker::transferDeposit(
    contracts::EscrowHandle& handle,
    const contracts::AgentId& recipientId,
    contracts::Money /*transferAmount*/) {
    if (!handle.isValid()) {
        return false;
    }
    eventLog_.log(contracts::SimTime(0), "ESCROW_TRANSFERRED", id(), recipientId, handle.contractId(), "Escrow deposit transferred");
    escrowLedger_.erase(handle.contractId());
    handle.invalidate();
    return true;
}

// IRecoveryContext
std::vector<contracts::AgentId> LogisticsBroker::findIdleTrucks(
    const contracts::NodeId& /*location*/,
    contracts::Quintals /*capacity*/) const {
    return {};
}

std::vector<contracts::MandiCandidate> LogisticsBroker::rankMandis(
    const contracts::CropBatchInfo& /*batch*/,
    const std::vector<contracts::AgentId>& /*exclude*/) const {
    return {};
}

std::vector<contracts::AgentId> LogisticsBroker::findProcessors(
    const contracts::NodeId& /*location*/,
    double /*radiusKm*/) const {
    return {};
}

contracts::RouteInfo LogisticsBroker::routeTo(
    const contracts::NodeId& from,
    const contracts::NodeId& to,
    contracts::RouteStrategyType /*strategy*/) const {
    return contracts::RouteInfo{
        .path = {from, to},
        .distanceKm = 50.0,
        .estimatedTransitTime = contracts::SimTime(60),
        .tollCost = contracts::Money(0),
        .freightCost = contracts::Money::fromRupees(1500.0)
    };
}

double LogisticsBroker::predictQuality(
    const contracts::CropBatchInfo& /*batch*/,
    const contracts::RouteInfo& /*route*/,
    double /*ambientTempC*/) const {
    return 0.80;
}

} // namespace kisansetu::agents
