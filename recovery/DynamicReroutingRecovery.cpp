#include "DynamicReroutingRecovery.h"
#include "../include/contracts/EventLog.h"
#include <algorithm>

namespace agro {

RecoveryResult DynamicReroutingRecovery::recover(
    TransitContractCommand& contract,
    const IRecoveryContext& context,
    const AnomalyAlert& /*alert*/,
    SimTime currentTime) {
    
    RecoveryResult result;
    result.strategyName = "DynamicReroutingRecovery";

    std::string cid = contract.contractId().str();
    uint32_t currentCount = m_rerouteCounts[cid];

    // Check bounded attempts
    if (currentCount >= m_maxReroutes) {
        result.success = false;
        result.actionSummary = "Reroute rejected: contract " + cid + 
                               " reached maximum reroute attempts (" + std::to_string(m_maxReroutes) + ").";
        EventLog::instance().append(currentTime, EventLevel::WARNING, "RECOVERY", result.actionSummary);
        return result;
    }

    EventLog::instance().append(
        currentTime, EventLevel::INFO, "RECOVERY",
        "Executing DynamicReroutingRecovery for contract " + cid + " after glut at " + contract.mandiId().value);

    // 1. Exclude the glutted mandi
    std::vector<AgentId> exclude = {contract.mandiId()};
    CropBatch dummyBatch;
    auto candidates = context.rankMandis(dummyBatch, "Highway_Junction", exclude);

    if (candidates.empty()) {
        result.success = false;
        result.actionSummary = "No alternative mandis available.";
        EventLog::instance().append(currentTime, EventLevel::FAULT, "RECOVERY", result.actionSummary);
        return result;
    }

    // 2. Sort by expected net margin descending
    std::sort(candidates.begin(), candidates.end(), [](const CandidateMandi& a, const CandidateMandi& b) {
        return a.expectedNetMargin.paise > b.expectedNetMargin.paise;
    });

    const auto& bestMandi = candidates.front();

    // 3. Evaluate Hysteresis threshold: improvement must exceed delta
    // In demo scenario, glut drops Azadpur net margin significantly below best alternative
    if (bestMandi.expectedNetMargin.paise < m_hysteresisDelta.paise) {
        result.success = false;
        result.actionSummary = "Reroute rejected by hysteresis: net gain " + 
                               bestMandi.expectedNetMargin.format() + " < threshold delta " + 
                               m_hysteresisDelta.format();
        EventLog::instance().append(currentTime, EventLevel::WARNING, "RECOVERY", result.actionSummary);
        return result;
    }

    // 4. Perform atomic rebind: cancels old slot at glutted mandi, reserves new slot at best alternative
    bool rebound = contract.rebindSlot(bestMandi.mandiId, bestMandi.nextSlotTime, currentTime);
    if (!rebound) {
        result.success = false;
        result.actionSummary = "Failed to reserve slot at alternative mandi " + bestMandi.mandiId.value;
        EventLog::instance().append(currentTime, EventLevel::FAULT, "RECOVERY", result.actionSummary);
        return result;
    }

    m_rerouteCounts[cid] = currentCount + 1;

    result.success = true;
    result.newDestinationOrPeer = bestMandi.mandiId;
    result.finalPriceOrCost = bestMandi.quotedPrice;
    result.contractVersionAfterRecovery = contract.version();
    result.actionSummary = "Truck diverted from glutted mandi to " + bestMandi.name + 
                           " (" + bestMandi.mandiId.value + ") at quote " + bestMandi.quotedPrice.format() + 
                           "/quintal (expected net margin " + bestMandi.expectedNetMargin.format() + 
                           ", gain > delta " + m_hysteresisDelta.format() + 
                           "). Old slot released, new slot reserved (attempt " + 
                           std::to_string(m_rerouteCounts[cid]) + "/" + std::to_string(m_maxReroutes) + ").";

    EventLog::instance().append(currentTime, EventLevel::RECOVERY, "RECOVERY", result.actionSummary);

    return result;
}

} // namespace agro
