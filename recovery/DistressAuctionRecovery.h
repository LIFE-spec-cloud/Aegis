#pragma once

#include "IFailureRecovery.h"

namespace agro {

class DistressAuctionRecovery : public IFailureRecovery {
public:
    explicit DistressAuctionRecovery(double maxRadiusKm = 30.0)
        : m_maxRadiusKm(maxRadiusKm) {}

    [[nodiscard]] RecoveryResult recover(
        TransitContractCommand& contract,
        const IRecoveryContext& context,
        const AnomalyAlert& alert,
        SimTime currentTime) override;

    void setMaxRadiusKm(double km) noexcept { m_maxRadiusKm = km; }
    [[nodiscard]] double maxRadiusKm() const noexcept { return m_maxRadiusKm; }

private:
    double m_maxRadiusKm{30.0};
};

} // namespace agro
