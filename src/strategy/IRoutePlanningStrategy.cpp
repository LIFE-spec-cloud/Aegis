#include "strategy/IRoutePlanningStrategy.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include <algorithm>

namespace kisansetu::strategy {

namespace {

class DijkstraRouteStrategy final : public IRoutePlanningStrategy {
public:
    [[nodiscard]] contracts::RouteStrategyType type() const noexcept override {
        return contracts::RouteStrategyType::DIJKSTRA;
    }

    [[nodiscard]] contracts::RouteInfo planRoute(
        const domain::RoadGraph& graph,
        const contracts::NodeId& origin,
        const contracts::NodeId& destination) const override {
        // Standard Dijkstra shortest travel time path
        std::unordered_map<contracts::NodeId, int64_t> dist;
        std::unordered_map<contracts::NodeId, contracts::NodeId> prev;
        std::unordered_map<contracts::NodeId, domain::RoadEdge> edgeTaken;

        if (!graph.hasNode(origin) || !graph.hasNode(destination)) {
            return contracts::RouteInfo{};
        }
        if (origin == destination) {
            return contracts::RouteInfo{.path = {origin}};
        }

        for (const auto& node : graph.nodes()) {
            dist[node] = std::numeric_limits<int64_t>::max();
        }
        dist[origin] = 0;

        using PQElement = std::pair<int64_t, contracts::NodeId>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;
        pq.push({0, origin});

        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();

            if (d > dist[u]) continue;
            if (u == destination) break;

            for (const auto& edge : graph.getEdgesFrom(u)) {
                int64_t weight = edge.effectiveTravelTime().ticks();
                if (dist[u] + weight < dist[edge.to]) {
                    dist[edge.to] = dist[u] + weight;
                    prev[edge.to] = u;
                    edgeTaken[edge.to] = edge;
                    pq.push({dist[edge.to], edge.to});
                }
            }
        }

        auto distIt = dist.find(destination);
        if (distIt == dist.end() || distIt->second == std::numeric_limits<int64_t>::max()) {
            return contracts::RouteInfo{};
        }

        std::vector<contracts::NodeId> path;
        contracts::NodeId curr = destination;
        double totalDist = 0.0;
        int64_t totalTicks = 0;
        contracts::Money totalTolls(0);

        while (curr != origin) {
            path.push_back(curr);
            auto edgeIt = edgeTaken.find(curr);
            auto prevIt = prev.find(curr);
            if (edgeIt == edgeTaken.end() || prevIt == prev.end()) {
                return contracts::RouteInfo{};
            }
            const auto& e = edgeIt->second;
            totalDist += e.distanceKm;
            totalTicks += e.effectiveTravelTime().ticks();
            totalTolls += e.tollCost;
            curr = prevIt->second;
        }
        path.push_back(origin);
        std::reverse(path.begin(), path.end());

        return contracts::RouteInfo{
            .path = path,
            .distanceKm = totalDist,
            .estimatedTransitTime = contracts::SimTime(totalTicks),
            .tollCost = totalTolls,
            .freightCost = contracts::Money::fromRupees(totalDist * 25.0) // ₹25/km base
        };
    }
};

class SpoilageMinimizingRouteStrategy final : public IRoutePlanningStrategy {
public:
    [[nodiscard]] contracts::RouteStrategyType type() const noexcept override {
        return contracts::RouteStrategyType::SPOILAGE_MINIMIZING;
    }

    [[nodiscard]] contracts::RouteInfo planRoute(
        const domain::RoadGraph& graph,
        const contracts::NodeId& origin,
        const contracts::NodeId& destination) const override {
        // Optimizes primarily for fastest travel time (minimum spoilage)
        DijkstraRouteStrategy fallback;
        return fallback.planRoute(graph, origin, destination);
    }
};

class TollAverseRouteStrategy final : public IRoutePlanningStrategy {
public:
    [[nodiscard]] contracts::RouteStrategyType type() const noexcept override {
        return contracts::RouteStrategyType::TOLL_AVERSE;
    }

    [[nodiscard]] contracts::RouteInfo planRoute(
        const domain::RoadGraph& graph,
        const contracts::NodeId& origin,
        const contracts::NodeId& destination) const override {
        // High penalty on toll routes
        std::unordered_map<contracts::NodeId, int64_t> cost;
        std::unordered_map<contracts::NodeId, contracts::NodeId> prev;
        std::unordered_map<contracts::NodeId, domain::RoadEdge> edgeTaken;

        if (!graph.hasNode(origin) || !graph.hasNode(destination)) {
            return contracts::RouteInfo{};
        }
        if (origin == destination) {
            return contracts::RouteInfo{.path = {origin}};
        }

        for (const auto& node : graph.nodes()) {
            cost[node] = std::numeric_limits<int64_t>::max();
        }
        cost[origin] = 0;

        using PQElement = std::pair<int64_t, contracts::NodeId>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;
        pq.push({0, origin});

        while (!pq.empty()) {
            auto [c, u] = pq.top();
            pq.pop();

            if (c > cost[u]) continue;
            if (u == destination) break;

            for (const auto& edge : graph.getEdgesFrom(u)) {
                // Cost = travel time + (toll in rupees * 10)
                int64_t tollPenalty = static_cast<int64_t>(edge.tollCost.toRupees() * 10.0);
                int64_t edgeCost = edge.effectiveTravelTime().ticks() + tollPenalty;
                if (cost[u] + edgeCost < cost[edge.to]) {
                    cost[edge.to] = cost[u] + edgeCost;
                    prev[edge.to] = u;
                    edgeTaken[edge.to] = edge;
                    pq.push({cost[edge.to], edge.to});
                }
            }
        }

        auto costIt = cost.find(destination);
        if (costIt == cost.end() || costIt->second == std::numeric_limits<int64_t>::max()) {
            return contracts::RouteInfo{};
        }

        std::vector<contracts::NodeId> path;
        contracts::NodeId curr = destination;
        double totalDist = 0.0;
        int64_t totalTicks = 0;
        contracts::Money totalTolls(0);

        while (curr != origin) {
            path.push_back(curr);
            auto edgeIt = edgeTaken.find(curr);
            auto prevIt = prev.find(curr);
            if (edgeIt == edgeTaken.end() || prevIt == prev.end()) {
                return contracts::RouteInfo{};
            }
            const auto& e = edgeIt->second;
            totalDist += e.distanceKm;
            totalTicks += e.effectiveTravelTime().ticks();
            totalTolls += e.tollCost;
            curr = prevIt->second;
        }
        path.push_back(origin);
        std::reverse(path.begin(), path.end());

        return contracts::RouteInfo{
            .path = path,
            .distanceKm = totalDist,
            .estimatedTransitTime = contracts::SimTime(totalTicks),
            .tollCost = totalTolls,
            .freightCost = contracts::Money::fromRupees(totalDist * 25.0)
        };
    }
};

} // namespace

std::unique_ptr<IRoutePlanningStrategy> IRoutePlanningStrategy::create(contracts::RouteStrategyType type) {
    switch (type) {
        case contracts::RouteStrategyType::DIJKSTRA:
            return std::make_unique<DijkstraRouteStrategy>();
        case contracts::RouteStrategyType::SPOILAGE_MINIMIZING:
            return std::make_unique<SpoilageMinimizingRouteStrategy>();
        case contracts::RouteStrategyType::TOLL_AVERSE:
            return std::make_unique<TollAverseRouteStrategy>();
    }
    return std::make_unique<DijkstraRouteStrategy>();
}

} // namespace kisansetu::strategy
