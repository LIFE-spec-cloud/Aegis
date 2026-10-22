#include "PeerHandoffRecovery.h"
#include "../include/contracts/EventLog.h"

namespace agro {

RecoveryResult PeerHandoffRecovery::recover(
    TransitContractCommand& contract,
    const IRecoveryContext& context,
    const AnomalyAlert& alert,
    SimTime currentTime) {
    
    RecoveryResult result;
    result.strategyName = "PeerHandoffRecovery";

    std::string cid = contract.contractId().value;
    std::string failedTruck = contract.transporterId().value;

    EventLog::instance().append(
        currentTime, EventLevel::INFO, "RECOVERY",
        "Executing PeerHandoffRecovery for contract " + cid + " after breakdown of " + failedTruck);

    // 1. Search for closest idle peer truck with sufficient capacity
    auto idleTrucks = context.findIdleTrucks("NH44_Junction", m_requiredCapacity);

    if (!idleTrucks.empty()) {
        const auto& peer = idleTrucks.front();

        // Rebind truck in contract command: transfers manifest, releases failed truck lock, binds peer truck lock
        bool rebound = contract.rebindTruck(peer.truckId, currentTime);
        if (rebound) {
            result.success = true;
            result.newDestinationOrPeer = peer.truckId;
            result.contractVersionAfterRecovery = contract.version();
            result.actionSummary = "Cargo manifest transferred from failed truck " + failedTruck + 
                                   " to idle peer truck " + peer.truckId.value + 
                                   " at rendezvous point NH44_Junction. Contract rebound; zero leaked locks.";
            
            EventLog::instance().append(currentTime, EventLevel::RECOVERY, "RECOVERY", result.actionSummary);
            return result;
        }
    }

    // 2. Fallback Chain: No idle truck in range -> Cold storage / Distress Auction
    EventLog::instance().append(
        currentTime, EventLevel::WARNING, "RECOVERY",
        "No idle truck in range for peer handoff. Activating fallback chain: cold storage -> distress auction.");

    if (m_fallbackRecovery) {
        return m_fallbackRecovery->recover(contract, context, alert, currentTime);
    }

    result.success = false;
    result.actionSummary = "Peer handoff failed and no fallback recovery configured.";
    EventLog::instance().append(currentTime, EventLevel::FAULT, "RECOVERY", result.actionSummary);
    return result;
}

} // namespace agro
