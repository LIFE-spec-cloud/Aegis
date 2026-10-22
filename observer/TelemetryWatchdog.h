#pragma once

#include "../include/contracts/ITelemetry.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>

namespace agro {

class TelemetryWatchdog {
public:
    TelemetryWatchdog() = default;

    // Observer registration
    void registerListener(ITelemetryWatchdogListener* listener);
    void unregisterListener(ITelemetryWatchdogListener* listener);
    [[nodiscard]] size_t listenerCount() const noexcept;

    // Ingest telemetry data and analyze for anomalies
    void processTruckTelemetry(const TelemetryEvent& event);
    void processMandiPriceTick(const MandiPriceTickEvent& event, Money farmerReservePrice);

    // Explicit anomaly trigger (from simulation or sensors)
    void triggerAnomaly(const AnomalyAlert& alert);

    // Configuration thresholds
    void setMinQualityFloor(double floor) noexcept { m_minQualityFloor = floor; }
    void setMaxHeartbeatLossTicks(uint64_t ticks) noexcept { m_maxHeartbeatLossTicks = ticks; }

    [[nodiscard]] double minQualityFloor() const noexcept { return m_minQualityFloor; }
    [[nodiscard]] uint64_t maxHeartbeatLossTicks() const noexcept { return m_maxHeartbeatLossTicks; }

private:
    void notifyTruckTelemetry(const TelemetryEvent& event);
    void notifyMandiPriceTick(const MandiPriceTickEvent& event);
    void notifyAnomalyAlert(const AnomalyAlert& alert);

    std::vector<ITelemetryWatchdogListener*> m_listeners;
    double m_minQualityFloor{0.60};        // Mandi reject threshold
    uint64_t m_maxHeartbeatLossTicks{3};   // 3 missed heartbeats = FAILED
    std::unordered_map<std::string, SimTime> m_lastHeartbeats;
};

} // namespace agro
