#pragma once

#include "IFailureRecovery.h"
#include <memory>

namespace agro {

class PeerHandoffRecovery : public IFailureRecovery {
public:
    explicit PeerHandoffRecovery(
        Quintals requiredCapacity = Quintals(50.0),
        std::shared_ptr<IFailureRecovery> fallbackRecovery = nullptr)
        : m_requiredCapacity(requiredCapacity),
          m_fallbackRecovery(std::move(fallbackRecovery)) {}

    [[nodiscard]] RecoveryResult recover(
        TransitContractCommand& contract,
        const IRecoveryContext& context,
        const AnomalyAlert& alert,
        SimTime currentTime) override;

    void setRequiredCapacity(Quintals cap) noexcept { m_requiredCapacity = cap; }
    void setFallbackRecovery(std::shared_ptr<IFailureRecovery> fallback) {
        m_fallbackRecovery = std::move(fallback);
    }

private:
    Quintals m_requiredCapacity{Quintals(50.0)};
    std::shared_ptr<IFailureRecovery> m_fallbackRecovery;
};

} // namespace agro
