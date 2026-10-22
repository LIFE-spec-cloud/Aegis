#include "TelemetryWatchdog.h"
#include "../include/contracts/EventLog.h"

namespace agro {

void TelemetryWatchdog::registerListener(ITelemetryWatchdogListener* listener) {
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end()) {
        m_listeners.push_back(listener);
    }
}

void TelemetryWatchdog::unregisterListener(ITelemetryWatchdogListener* listener) {
    m_listeners.erase(
        std::remove(m_listeners.begin(), m_listeners.end(), listener),
        m_listeners.end());
}

size_t TelemetryWatchdog::listenerCount() const noexcept {
    return m_listeners.size();
}

void TelemetryWatchdog::processTruckTelemetry(const TelemetryEvent& event) {
    notifyTruckTelemetry(event);

    // Anomaly Check 1: Explicit FAILED state
    if (event.state == TruckState::FAILED) {
        AnomalyAlert alert;
        alert.type = AnomalyType::TransporterFailure;
        alert.contractId = event.contractId;
        alert.agentId = event.truckId;
        alert.detectedAt = event.currentSimTime;
        alert.description = "Transporter " + event.truckId.value + " entered FAILED mechanical state.";
        triggerAnomaly(alert);
        return;
    }

    // Anomaly Check 2: Missed Heartbeats
    if (event.currentSimTime.ticks >= event.lastHeartbeatTime.ticks + m_maxHeartbeatLossTicks &&
        event.lastHeartbeatTime.ticks > 0) {
        AnomalyAlert alert;
        alert.type = AnomalyType::TransporterFailure;
        alert.contractId = event.contractId;
        alert.agentId = event.truckId;
        alert.detectedAt = event.currentSimTime;
        alert.description = "Transporter " + event.truckId.value + " missed " + 
                            std::to_string(m_maxHeartbeatLossTicks) + " consecutive heartbeats.";
        triggerAnomaly(alert);
        return;
    }
    m_lastHeartbeats[event.truckId.value] = event.lastHeartbeatTime;

    // Anomaly Check 3: Predicted arrival quality breach below mandi threshold
    if (event.predictedQualityAtArrival < m_minQualityFloor && event.predictedQualityAtArrival > 0.0) {
        AnomalyAlert alert;
        alert.type = AnomalyType::HighwayStoppage;
        alert.contractId = event.contractId;
        alert.agentId = event.truckId;
        alert.detectedAt = event.currentSimTime;
        alert.description = "Quality breach predicted at destination: Q=" + 
                            std::to_string(event.predictedQualityAtArrival) + 
                            " < floor " + std::to_string(m_minQualityFloor) + 
                            " (highway transit delay).";
        triggerAnomaly(alert);
    }
}

void TelemetryWatchdog::processMandiPriceTick(const MandiPriceTickEvent& event, Money farmerReservePrice) {
    notifyMandiPriceTick(event);

    // Anomaly Check: Mandi glut / price crash below farmer reserve
    if (event.quotedPrice.paise < farmerReservePrice.paise && farmerReservePrice.paise > 0) {
        AnomalyAlert alert;
        alert.type = AnomalyType::MandiGlut;
        alert.agentId = event.mandiId;
        alert.detectedAt = event.timestamp;
        alert.description = "Mandi " + event.mandiId.value + " price crashed to " + 
                            event.quotedPrice.format() + ", below farmer reserve " + 
                            farmerReservePrice.format() + " (supply surge glut).";
        triggerAnomaly(alert);
    }
}

void TelemetryWatchdog::triggerAnomaly(const AnomalyAlert& alert) {
    EventLog::instance().append(
        alert.detectedAt, EventLevel::FAULT, "WATCHDOG",
        "ANOMALY DETECTED [" + alert.contractId.value + " / " + alert.agentId.value + "]: " + alert.description);
    notifyAnomalyAlert(alert);
}

void TelemetryWatchdog::notifyTruckTelemetry(const TelemetryEvent& event) {
    for (auto* listener : m_listeners) {
        if (listener) listener->onTruckTelemetry(event);
    }
}

void TelemetryWatchdog::notifyMandiPriceTick(const MandiPriceTickEvent& event) {
    for (auto* listener : m_listeners) {
        if (listener) listener->onMandiPriceTick(event);
    }
}

void TelemetryWatchdog::notifyAnomalyAlert(const AnomalyAlert& alert) {
    for (auto* listener : m_listeners) {
        if (listener) listener->onAnomalyAlert(alert);
    }
}

} // namespace agro
