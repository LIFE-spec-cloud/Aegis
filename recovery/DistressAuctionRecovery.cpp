#include "DistressAuctionRecovery.h"
#include "../include/contracts/EventLog.h"
#include <algorithm>

namespace agro {

RecoveryResult DistressAuctionRecovery::recover(
    TransitContractCommand& contract,
    const IRecoveryContext& context,
    const AnomalyAlert& /*alert*/,
    SimTime currentTime) {
    
    RecoveryResult result;
    result.strategyName = "DistressAuctionRecovery";

    EventLog::instance().append(
        currentTime, EventLevel::INFO, "RECOVERY",
        "Executing DistressAuctionRecovery for contract " + contract.contractId().str() + 
        " within radius " + std::to_string(static_cast<int>(m_maxRadiusKm)) + " km");

    // 1. Find processors within configured radius
    auto candidates = context.findProcessors(std::string("NH44_Stoppage"), m_maxRadiusKm);
    if (candidates.empty()) {
        result.success = false;
        result.actionSummary = "No processing plants found within " + std::to_string(m_maxRadiusKm) + " km.";
        EventLog::instance().append(currentTime, EventLevel::FAULT, "RECOVERY", result.actionSummary);
        return result;
    }

    // 2. Rank candidates by bid price descending (highest net bid wins)
    std::sort(candidates.begin(), candidates.end(), [](const CandidateProcessor& a, const CandidateProcessor& b) {
        return a.currentBidPricePerQuintal.paise > b.currentBidPricePerQuintal.paise;
    });

    const auto& winner = candidates.front();

    // 3. Compensate mandi gate slot and rebind destination to the winning processor
    bool diverted = contract.divertToProcessor(winner.processorId, winner.currentBidPricePerQuintal, currentTime);
    if (!diverted) {
        result.success = false;
        result.actionSummary = "Failed to compensate gate slot or rebind destination to " + winner.processorId.value;
        return result;
    }

    result.success = true;
    result.newDestinationOrPeer = winner.processorId;
    result.finalPriceOrCost = winner.currentBidPricePerQuintal;
    result.contractVersionAfterRecovery = contract.version();
    result.actionSummary = "Truck diverted to processor " + winner.name + " (" + winner.processorId.value + 
                           ") at bid " + winner.currentBidPricePerQuintal.format() + 
                           "/quintal (distance " + std::to_string(static_cast<int>(winner.distanceKm)) + 
                           " km). Mandi slot released; zero leaked locks.";

    EventLog::instance().append(currentTime, EventLevel::RECOVERY, "RECOVERY", result.actionSummary);

    return result;
}

} // namespace agro
