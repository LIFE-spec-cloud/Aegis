#pragma once

#include "IFailureRecovery.h"
#include <unordered_map>

namespace agro {

class DynamicReroutingRecovery : public IFailureRecovery {
public:
    explicit DynamicReroutingRecovery(
        Money hysteresisDelta = Money::fromRupees(5000),
        uint32_t maxReroutes = 2)
        : m_hysteresisDelta(hysteresisDelta),
          m_maxReroutes(maxReroutes) {}

    [[nodiscard]] RecoveryResult recover(
        TransitContractCommand& contract,
        const IRecoveryContext& context,
        const AnomalyAlert& alert,
        SimTime currentTime) override;

    void setHysteresisDelta(Money delta) noexcept { m_hysteresisDelta = delta; }
    void setMaxReroutes(uint32_t max) noexcept { m_maxReroutes = max; }

    [[nodiscard]] Money hysteresisDelta() const noexcept { return m_hysteresisDelta; }
    [[nodiscard]] uint32_t maxReroutes() const noexcept { return m_maxReroutes; }
    [[nodiscard]] uint32_t rerouteCount(const std::string& contractId) const {
        auto it = m_rerouteCounts.find(contractId);
        return it != m_rerouteCounts.end() ? it->second : 0;
    }

private:
    Money m_hysteresisDelta{Money::fromRupees(5000)};
    uint32_t m_maxReroutes{2};
    std::unordered_map<std::string, uint32_t> m_rerouteCounts;
};

} // namespace agro
