#pragma once

#include "contracts/Types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace kisansetu::domain {

/**
 * @brief Directed edge on the transport road network with mutable weights.
 */
struct RoadEdge {
    contracts::NodeId from;
    contracts::NodeId to;
    double distanceKm{0.0};
    contracts::SimTime baseTravelTime{0};
    contracts::Money tollCost{0};
    double congestionMultiplier{1.0};

    [[nodiscard]] contracts::SimTime effectiveTravelTime() const {
        return contracts::SimTime(static_cast<int64_t>(
            baseTravelTime.ticks() * congestionMultiplier));
    }
};

/**
 * @brief Road graph with runtime-mutable weights for congestion, tolls, and closures.
 */
class RoadGraph {
public:
    RoadGraph() = default;

    void addNode(contracts::NodeId node);
    void addEdge(contracts::NodeId from,
                 contracts::NodeId to,
                 double distanceKm,
                 contracts::SimTime travelTime,
                 contracts::Money tollCost);

    void setCongestion(const contracts::NodeId& from,
                       const contracts::NodeId& to,
                       double multiplier);

    [[nodiscard]] bool hasNode(const contracts::NodeId& node) const;
    [[nodiscard]] const std::vector<contracts::NodeId>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] std::vector<RoadEdge> getEdgesFrom(const contracts::NodeId& node) const;
    [[nodiscard]] std::optional<RoadEdge> getEdge(const contracts::NodeId& from, const contracts::NodeId& to) const;

    static RoadGraph createDefaultDemoWorld();

private:
    std::vector<contracts::NodeId> nodes_;
    std::unordered_map<contracts::NodeId, std::vector<RoadEdge>> adjacency_;
};

} // namespace kisansetu::domain
