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
    AgentId truckId;
    NodeId position;
    double speedKmh{0.0};
    TruckState state{TruckState::IDLE};
    SimTime predictedArrival{0};
    double currentQuality{1.0};
    std::string batchId;
    std::optional<MandiPriceTickPayload> mandiPriceTick;

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

/**
 * @brief Anomaly classification recognized by TelemetryWatchdog.
 */
enum class AnomalyType {
    SPOILAGE_RISK,      // Q_arrival < mandi_min_quality (0.60)
    MANDI_GLUT,          // Severe price depression below farmer reserve
    TRUCK_STALL,         // Truck stopped or stalled
    HEARTBEAT_TIMEOUT    // Missing heartbeat for 3 ticks
};

inline std::string toString(AnomalyType anomaly) {
    switch (anomaly) {
        case AnomalyType::SPOILAGE_RISK:   return "SPOILAGE_RISK";
        case AnomalyType::MANDI_GLUT:       return "MANDI_GLUT";
        case AnomalyType::TRUCK_STALL:      return "TRUCK_STALL";
        case AnomalyType::HEARTBEAT_TIMEOUT: return "HEARTBEAT_TIMEOUT";
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
};

/**
 * @brief Observer interface for listening to telemetry and watchdog anomalies.
 */
class ITelemetryWatchdogListener {
public:
    virtual ~ITelemetryWatchdogListener() = default;

    virtual void onTelemetry(const TelemetryEvent& event) = 0;
    virtual void onAnomaly(const AnomalyEvent& anomaly) = 0;
};

} // namespace kisansetu::contracts
