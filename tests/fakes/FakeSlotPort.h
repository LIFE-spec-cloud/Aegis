#pragma once

#include "../../include/contracts/IReservationPorts.h"
#include <unordered_map>
#include <sstream>

namespace agro::fakes {

class FakeSlotPort : public ISlotPort {
public:
    void setFailReservations(bool fail) noexcept { m_failReservations = fail; }
    void setFailCancellations(bool fail) noexcept { m_failCancellations = fail; }

    [[nodiscard]] std::optional<SlotReservationHandle> reserveSlot(
        const AgentId& mandiId,
        SimTime slotTime,
        const ContractId& contractId) override {
        
        if (m_failReservations) {
            return std::nullopt;
        }

        std::string slotKey = mandiId.value + "@" + std::to_string(slotTime.ticks());
        if (m_reservedSlots.find(slotKey) != m_reservedSlots.end()) {
            return std::nullopt; // Slot already taken
        }

        std::ostringstream oss;
        oss << "slot_res_" << (++m_sequence) << "_" << mandiId.value;
        std::string resId = oss.str();

        m_reservedSlots[slotKey] = resId;
        m_activeReservations[resId] = slotKey;

        return SlotReservationHandle(resId, mandiId, slotTime, contractId);
    }

    [[nodiscard]] bool cancelSlot(const SlotReservationHandle& handle) override {
        if (!handle.isValid()) return false;
        if (m_failCancellations) return false;

        auto it = m_activeReservations.find(handle.reservationIdStr());
        if (it != m_activeReservations.end()) {
            m_reservedSlots.erase(it->second);
            m_activeReservations.erase(it);
            return true;
        }
        return false;
    }

    [[nodiscard]] size_t activeReservationCount() const noexcept {
        return m_activeReservations.size();
    }

    [[nodiscard]] bool isSlotReserved(const AgentId& mandiId, SimTime slotTime) const {
        std::string slotKey = mandiId.value + "@" + std::to_string(slotTime.ticks());
        return m_reservedSlots.find(slotKey) != m_reservedSlots.end();
    }

private:
    bool m_failReservations{false};
    bool m_failCancellations{false};
    uint64_t m_sequence{0};
    std::unordered_map<std::string, std::string> m_reservedSlots;     // slotKey -> resId
    std::unordered_map<std::string, std::string> m_activeReservations; // resId -> slotKey
};

} // namespace agro::fakes
