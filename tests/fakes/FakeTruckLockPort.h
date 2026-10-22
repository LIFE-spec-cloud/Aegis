#pragma once

#include "../../include/contracts/IReservationPorts.h"
#include <unordered_map>
#include <sstream>

namespace agro::fakes {

class FakeTruckLockPort : public ITruckLockPort {
public:
    void setFailLocks(bool fail) noexcept { m_failLocks = fail; }
    void setFailReleases(bool fail) noexcept { m_failReleases = fail; }
    void setFailRebinds(bool fail) noexcept { m_failRebinds = fail; }

    [[nodiscard]] std::optional<TruckLockHandle> lockTruck(
        const AgentId& truckId,
        const ContractId& contractId) override {
        
        if (m_failLocks) {
            return std::nullopt;
        }

        if (m_lockedTrucks.find(truckId.value) != m_lockedTrucks.end()) {
            return std::nullopt; // Truck already locked
        }

        std::ostringstream oss;
        oss << "truck_lock_" << (++m_sequence) << "_" << truckId.value;
        std::string lockId = oss.str();

        m_lockedTrucks[truckId.value] = lockId;
        m_activeLocks[lockId] = truckId.value;

        return TruckLockHandle(lockId, truckId, contractId);
    }

    [[nodiscard]] bool releaseTruck(const TruckLockHandle& handle) override {
        if (!handle.isValid()) return false;
        if (m_failReleases) return false;

        auto it = m_activeLocks.find(handle.lockIdStr());
        if (it != m_activeLocks.end()) {
            m_lockedTrucks.erase(it->second);
            m_activeLocks.erase(it);
            return true;
        }
        return false;
    }

    [[nodiscard]] bool rebindTruck(
        TruckLockHandle& handle,
        const AgentId& newTruckId) override {
        
        if (!handle.isValid()) return false;
        if (m_failRebinds) return false;

        // Check if new truck is already locked
        if (m_lockedTrucks.find(newTruckId.value) != m_lockedTrucks.end()) {
            return false;
        }

        // Release old truck
        auto it = m_activeLocks.find(handle.lockIdStr());
        if (it != m_activeLocks.end()) {
            m_lockedTrucks.erase(it->second);
            // Re-bind to new truck
            m_lockedTrucks[newTruckId.value] = handle.lockIdStr();
            m_activeLocks[handle.lockIdStr()] = newTruckId.value;
            handle.setTruckId(newTruckId);
            return true;
        }

        return false;
    }

    [[nodiscard]] size_t activeLockCount() const noexcept {
        return m_activeLocks.size();
    }

    [[nodiscard]] bool isTruckLocked(const AgentId& truckId) const {
        return m_lockedTrucks.find(truckId.value) != m_lockedTrucks.end();
    }

private:
    bool m_failLocks{false};
    bool m_failReleases{false};
    bool m_failRebinds{false};
    uint64_t m_sequence{0};
    std::unordered_map<std::string, std::string> m_lockedTrucks; // truckId -> lockId
    std::unordered_map<std::string, std::string> m_activeLocks;  // lockId -> truckId
};

} // namespace agro::fakes
