#pragma once

#include "Types.h"
#include <string>

namespace agro {

struct TelemetryEvent {
    AgentId truckId;
    ContractId contractId;
    std::string currentNode;
    double speedKmH{0.0};
    TruckState state{TruckState::IDLE};
    SimTime currentSimTime{0};
    SimTime estimatedArrivalSimTime{0};
    CropBatch cargo;
    double currentQuality{1.0};
    double predictedQualityAtArrival{1.0};
    SimTime lastHeartbeatTime{0};
};

struct MandiPriceTickEvent {
    AgentId mandiId;
    CommodityType commodity{CommodityType::Tomato};
    Money quotedPrice;
    Money baselinePrice;
    double surgeRatio{1.0};
    uint32_t activeArrivalsCount{0};
    uint32_t hourlyCapacity{4};
    SimTime timestamp{0};
};

enum class AnomalyType {
    HighwayStoppage,     // transit delay leading to quality breach
    MandiGlut,           // price crash below reserve / alternative better by delta
    TransporterFailure,  // truck failed or missed heartbeats
    StaleTelemetry
};

struct AnomalyAlert {
    AnomalyType type;
    ContractId contractId;
    AgentId agentId;
    std::string description;
    SimTime detectedAt{0};
};

class ITelemetryWatchdogListener {
public:
    virtual ~ITelemetryWatchdogListener() = default;
    virtual void onTruckTelemetry(const TelemetryEvent& event) = 0;
    virtual void onMandiPriceTick(const MandiPriceTickEvent& event) = 0;
    virtual void onAnomalyAlert(const AnomalyAlert& alert) = 0;
};

} // namespace agro
