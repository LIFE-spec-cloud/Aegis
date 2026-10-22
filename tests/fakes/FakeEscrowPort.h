#pragma once

#include "../../include/contracts/IReservationPorts.h"
#include <unordered_map>
#include <sstream>

namespace agro::fakes {

class FakeEscrowPort : public IEscrowPort {
public:
    void setFailHolds(bool fail) noexcept { m_failHolds = fail; }
    void setFailRefunds(bool fail) noexcept { m_failRefunds = fail; }
    void setFailTransfers(bool fail) noexcept { m_failTransfers = fail; }

    [[nodiscard]] std::optional<EscrowHandle> holdDeposit(
        const AgentId& payerId,
        Money amount,
        const ContractId& contractId) override {
        
        if (m_failHolds) {
            return std::nullopt;
        }

        std::ostringstream oss;
        oss << "escrow_" << (++m_sequence) << "_" << payerId.value;
        std::string escrowId = oss.str();

        m_heldEscrows[escrowId] = amount;
        m_totalEscrowHeld += amount;

        return EscrowHandle(escrowId, payerId, amount, contractId);
    }

    [[nodiscard]] bool refundEscrow(
        const EscrowHandle& handle,
        Money refundAmount) override {
        
        if (!handle.isValid()) return false;
        if (m_failRefunds) return false;

        auto it = m_heldEscrows.find(handle.escrowId());
        if (it != m_heldEscrows.end()) {
            if (refundAmount.paise > it->second.paise) {
                return false; // Cannot refund more than held
            }
            it->second -= refundAmount;
            m_totalEscrowHeld -= refundAmount;
            m_totalRefunded += refundAmount;
            if (it->second.paise == 0) {
                m_heldEscrows.erase(it);
            }
            return true;
        }
        return false;
    }

    [[nodiscard]] bool transferEscrow(
        const EscrowHandle& handle,
        const AgentId& /*toAgent*/,
        Money amount) override {
        
        if (!handle.isValid()) return false;
        if (m_failTransfers) return false;

        auto it = m_heldEscrows.find(handle.escrowId());
        if (it != m_heldEscrows.end()) {
            if (amount.paise > it->second.paise) {
                return false;
            }
            it->second -= amount;
            m_totalEscrowHeld -= amount;
            m_totalTransferred += amount;
            if (it->second.paise == 0) {
                m_heldEscrows.erase(it);
            }
            return true;
        }
        return false;
    }

    [[nodiscard]] Money totalHeld() const noexcept { return m_totalEscrowHeld; }
    [[nodiscard]] Money totalRefunded() const noexcept { return m_totalRefunded; }
    [[nodiscard]] Money totalTransferred() const noexcept { return m_totalTransferred; }
    [[nodiscard]] size_t activeEscrowCount() const noexcept { return m_heldEscrows.size(); }

private:
    bool m_failHolds{false};
    bool m_failRefunds{false};
    bool m_failTransfers{false};
    uint64_t m_sequence{0};
    Money m_totalEscrowHeld{0};
    Money m_totalRefunded{0};
    Money m_totalTransferred{0};
    std::unordered_map<std::string, Money> m_heldEscrows; // escrowId -> current balance
};

} // namespace agro::fakes
