#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <optional>

namespace agro {

struct CandidateTruck {
    AgentId truckId;
    std::string currentNode;
    Quintals capacity{0.0};
    SimTime availableFrom{0};
    double costPerKm{0.0};
};

struct CandidateMandi {
    AgentId mandiId;
    std::string name;
    Money quotedPrice;
    Money expectedNetMargin;
    SimTime nextSlotTime{0};
    double distanceKm{0.0};
    SimTime estimatedTransitTime{0};
    double predictedArrivalQuality{1.0};
};

struct CandidateProcessor {
    AgentId processorId;
    std::string name;
    std::string locationNode;
    Money priceFloorPerQuintal;
    Money currentBidPricePerQuintal;
    double distanceKm{0.0};
    SimTime estimatedTransitTime{0};
    double predictedArrivalQuality{1.0};
};

struct RouteInfo {
    std::vector<std::string> pathNodes;
    double totalDistanceKm{0.0};
    SimTime estimatedDuration{0};
    Money tollAndFuelCost{0};
};

class IRecoveryContext {
public:
    virtual ~IRecoveryContext() = default;

    [[nodiscard]] virtual std::vector<CandidateTruck> findIdleTrucks(
        const std::string& locationNode,
        Quintals requiredCapacity) const = 0;

    [[nodiscard]] virtual std::vector<CandidateMandi> rankMandis(
        const CropBatch& batch,
        const std::string& fromNode,
        const std::vector<AgentId>& excludeMandis) const = 0;

    [[nodiscard]] virtual std::vector<CandidateProcessor> findProcessors(
        const std::string& locationNode,
        double radiusKm) const = 0;

    [[nodiscard]] virtual RouteInfo routeTo(
        const std::string& fromNode,
        const std::string& toNode) const = 0;

    [[nodiscard]] virtual double predictQuality(
        const CropBatch& batch,
        SimTime transitTime) const = 0;
};

} // namespace agro
