#include "agents/APMCMandiAgent.h"

namespace kisansetu::agents {

void APMCMandiAgent::onMessage(const contracts::Envelope& /*envelope*/) {
    // Stub for A0; fully implemented in A3
}

std::optional<contracts::SlotReservationHandle> APMCMandiAgent::reserveSlot(
    const contracts::AgentId& mandiId,
    const contracts::ContractId& contractId,
    contracts::SimTime slotTime) {
    if (mandiId != id()) {
        return std::nullopt;
    }

    int64_t hour = slotTime.ticks() / 60;
    auto& slots = hourlySlots_[hour];
    if (slots.size() >= gateQuotaPerHour_) {
        return std::nullopt; // Quota full
    }

    slots.push_back(contractId);
    return contracts::SlotReservationHandle(
        id(), contractId, slotTime, ++nextSlotReservationId_);
}

bool APMCMandiAgent::cancelSlot(contracts::SlotReservationHandle& handle) {
    if (!handle.isValid() || handle.mandiId() != id()) {
        return false;
    }

    int64_t hour = handle.slotTime().ticks() / 60;
    auto it = hourlySlots_.find(hour);
    if (it != hourlySlots_.end()) {
        auto& vec = it->second;
        for (auto vIt = vec.begin(); vIt != vec.end(); ++vIt) {
            if (*vIt == handle.contractId()) {
                vec.erase(vIt);
                break;
            }
        }
    }

    handle.invalidate();
    return true;
}

} // namespace kisansetu::agents
