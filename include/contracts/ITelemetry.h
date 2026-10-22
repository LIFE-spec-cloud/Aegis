#pragma once

#include "Types.h"
#include "Payloads.h"
#include <string>
#include <optional>
#include <sstream>
#include <iomanip>

namespace kisansetu::contracts {

/**
 * @brief Telemetry snapshot emitted periodically by transporters or market ticks.
 */
struct TelemetryEvent {
    SimTime timestamp{0};
    AgentId truckId{};
    NodeId position{"Farm_Sonipat"};
    double speedKmh{0.0};
    TruckState state{TruckState::IDLE};
    SimTime predictedArrival{0};
    double currentQuality{1.0};
    std::string batchId{};
    std::optional<MandiPriceTickPayload> mandiPriceTick{std::nullopt};

    ContractId contractId{0};
    std::string currentNode{"Farm_Sonipat"};
    double speedKmH{0.0};
    SimTime currentSimTime{0};
    SimTime estimatedArrivalSimTime{0};
    double predictedQualityAtArrival{1.0};
    SimTime lastHeartbeatTime{0};

    [[nodiscard]] std::string toLogString() const {
        std::ostringstream oss;
        oss << "[Telemetry @" << timestamp.ticks() << "m] Truck=" << truckId
            << " Pos=" << position << " Speed=" << std::fixed << std::setprecision(1) << speedKmh << "km/h"
            << " State=" << toString(state)
            << " ETA=" << predictedArrival.ticks() << "m"
            << " Q=" << std::fixed << std::setprecision(3) << currentQuality;
        if (mandiPriceTick.has_value()) {
            oss << " | MandiTick=" << mandiPriceTick->mandiId
                << " P=" << mandiPriceTick->currentPricePerQuintal
                << " Glut=" << (mandiPriceTick->glutFlag ? "YES" : "NO");
        }
        return oss.str();
    }
};

struct MandiPriceTickEvent {
    AgentId mandiId;
    CommodityType commodity{CommodityType::Tomato};
    Money quotedPrice{0};
    Money baselinePrice{0};
    double surgeRatio{1.0};
    uint32_t activeArrivalsCount{0};
    uint32_t hourlyCapacity{4};
    SimTime timestamp{0};
};

/**
 * @brief Anomaly classification recognized by TelemetryWatchdog.
 */
enum class AnomalyType {
    SPOILAGE_RISK,
    MANDI_GLUT,
    TRUCK_STALL,
    HEARTBEAT_TIMEOUT,

    // Aliases
    HighwayStoppage = SPOILAGE_RISK,
    MandiGlut = MANDI_GLUT,
    TransporterFailure = TRUCK_STALL,
    StaleTelemetry = HEARTBEAT_TIMEOUT
};

inline std::string toString(AnomalyType anomaly) {
    switch (anomaly) {
        case AnomalyType::SPOILAGE_RISK:     return "SPOILAGE_RISK";
        case AnomalyType::MANDI_GLUT:         return "MANDI_GLUT";
        case AnomalyType::TRUCK_STALL:        return "TRUCK_STALL";
        case AnomalyType::HEARTBEAT_TIMEOUT:  return "HEARTBEAT_TIMEOUT";
    }
    return "Unknown";
}

/**
 * @brief Anomaly event raised by the watchdog.
 */
struct AnomalyEvent {
    SimTime timestamp{0};
    AnomalyType type{AnomalyType::SPOILAGE_RISK};
    AgentId agentId;
    ContractId contractId{0};
    std::string description;
    SimTime detectedAt{0};
};

using AnomalyAlert = AnomalyEvent;

/**
 * @brief Observer interface for listening to telemetry and watchdog anomalies.
 */
class ITelemetryWatchdogListener {
public:
    virtual ~ITelemetryWatchdogListener() = default;

    virtual void onTelemetry(const TelemetryEvent& event) { (void)event; }
    virtual void onAnomaly(const AnomalyEvent& anomaly) { (void)anomaly; }

    virtual void onTruckTelemetry(const TelemetryEvent& event) { onTelemetry(event); }
    virtual void onMandiPriceTick(const MandiPriceTickEvent& event) { (void)event; }
    virtual void onAnomalyAlert(const AnomalyAlert& alert) { onAnomaly(alert); }
};

} // namespace kisansetu::contracts
