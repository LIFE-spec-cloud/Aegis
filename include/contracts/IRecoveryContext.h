#pragma once

#include "Types.h"
#include <vector>
#include <string>

namespace kisansetu::contracts {

/**
 * @brief Routing optimization strategy types.
 */
enum class RouteStrategyType {
    DIJKSTRA,
    SPOILAGE_MINIMIZING,
    TOLL_AVERSE
};

inline std::string toString(RouteStrategyType strat) {
    switch (strat) {
        case RouteStrategyType::DIJKSTRA:            return "DIJKSTRA";
        case RouteStrategyType::SPOILAGE_MINIMIZING: return "SPOILAGE_MINIMIZING";
        case RouteStrategyType::TOLL_AVERSE:         return "TOLL_AVERSE";
    }
    return "Unknown";
}

/**
 * @brief Calculated route details across the road graph.
 */
struct RouteInfo {
    std::vector<NodeId> path;
    double distanceKm{0.0};
    SimTime estimatedTransitTime{0};
    Money tollCost{0};
    Money freightCost{0};
};

/**
 * @brief Crop batch metadata used during routing and recovery evaluations.
 */
struct CropBatchInfo {
    std::string batchId;
    CommodityType crop{CommodityType::Tomato};
    Quintals volume{0.0};
    double initialQuality{1.0};
    SimTime harvestTime{0};
    bool isRefrigerated{false};
};

/**
 * @brief Mandi option evaluated for rerouting.
 */
struct MandiCandidate {
    AgentId mandiId;
    NodeId location;
    Money forecastPrice{0};
    Money netMargin{0};
    SimTime eta{0};
    double predictedArrivalQuality{1.0};
};

/**
 * @brief Read-only query context provided by LogisticsBroker to recovery strategies.
 */
class IRecoveryContext {
public:
    virtual ~IRecoveryContext() = default;

    /**
     * @brief Find idle trucks capable of carrying the specified capacity.
     */
    [[nodiscard]] virtual std::vector<AgentId> findIdleTrucks(
        const NodeId& location,
        Quintals capacity) const = 0;

    /**
     * @brief Rank available mandis for rerouting by risk-adjusted net margin.
     */
    [[nodiscard]] virtual std::vector<MandiCandidate> rankMandis(
        const CropBatchInfo& batch,
        const std::vector<AgentId>& exclude) const = 0;

    /**
     * @brief Locate food processing plants within the given radius from a location.
     */
    [[nodiscard]] virtual std::vector<AgentId> findProcessors(
        const NodeId& location,
        double radiusKm) const = 0;

    /**
     * @brief Compute the optimal path between two nodes using the selected strategy.
     */
    [[nodiscard]] virtual RouteInfo routeTo(
        const NodeId& from,
        const NodeId& to,
        RouteStrategyType strategy) const = 0;

    /**
     * @brief Predict the arrival quality Q of a batch along a specified route.
     */
    [[nodiscard]] virtual double predictQuality(
        const CropBatchInfo& batch,
        const RouteInfo& route,
        double ambientTempC) const = 0;
};

} // namespace kisansetu::contracts
