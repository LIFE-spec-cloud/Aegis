#include "domain/RoadGraph.h"
#include <algorithm>

namespace kisansetu::domain {

void RoadGraph::addNode(contracts::NodeId node) {
    if (!hasNode(node)) {
        nodes_.push_back(node);
        adjacency_[node] = {};
    }
}

void RoadGraph::addEdge(contracts::NodeId from,
                        contracts::NodeId to,
                        double distanceKm,
                        contracts::SimTime travelTime,
                        contracts::Money tollCost) {
    addNode(from);
    addNode(to);

    RoadEdge edge{
        .from = from,
        .to = to,
        .distanceKm = distanceKm,
        .baseTravelTime = travelTime,
        .tollCost = tollCost,
        .congestionMultiplier = 1.0
    };

    adjacency_[from].push_back(edge);
}

void RoadGraph::addBidirectionalEdge(contracts::NodeId u,
                                    contracts::NodeId v,
                                    double distanceKm,
                                    contracts::SimTime travelTime,
                                    contracts::Money tollCost) {
    addEdge(u, v, distanceKm, travelTime, tollCost);
    addEdge(v, u, distanceKm, travelTime, tollCost);
}

void RoadGraph::setCongestion(const contracts::NodeId& from,
                             const contracts::NodeId& to,
                             double multiplier) {
    auto it = adjacency_.find(from);
    if (it != adjacency_.end()) {
        for (auto& edge : it->second) {
            if (edge.to == to) {
                edge.congestionMultiplier = multiplier;
            }
        }
    }
}

void RoadGraph::setToll(const contracts::NodeId& from,
                       const contracts::NodeId& to,
                       contracts::Money newToll) {
    auto it = adjacency_.find(from);
    if (it != adjacency_.end()) {
        for (auto& edge : it->second) {
            if (edge.to == to) {
                edge.tollCost = newToll;
            }
        }
    }
}

void RoadGraph::setTravelTime(const contracts::NodeId& from,
                             const contracts::NodeId& to,
                             contracts::SimTime newTravelTime) {
    auto it = adjacency_.find(from);
    if (it != adjacency_.end()) {
        for (auto& edge : it->second) {
            if (edge.to == to) {
                edge.baseTravelTime = newTravelTime;
            }
        }
    }
}

bool RoadGraph::hasNode(const contracts::NodeId& node) const {
    return std::find(nodes_.begin(), nodes_.end(), node) != nodes_.end();
}

std::vector<RoadEdge> RoadGraph::getEdgesFrom(const contracts::NodeId& node) const {
    auto it = adjacency_.find(node);
    if (it != adjacency_.end()) {
        return it->second;
    }
    return {};
}

std::optional<RoadEdge> RoadGraph::getEdge(const contracts::NodeId& from, const contracts::NodeId& to) const {
    auto it = adjacency_.find(from);
    if (it != adjacency_.end()) {
        for (const auto& edge : it->second) {
            if (edge.to == to) {
                return edge;
            }
        }
    }
    return std::nullopt;
}

RoadGraph RoadGraph::createDefaultDemoWorld() {
    RoadGraph graph;

    // 12 Nodes:
    // Farm clusters: Farm_Sonipat, Farm_Nashik, Farm_KolarRural
    // Junctions: Jct_Panipat, Jct_Thane, Jct_Hosur, Jct_NH44
    // Mandis: Mandi_Azadpur, Mandi_Ghazipur, Mandi_Vashi, Mandi_Kolar
    // Processing Plants: Plant_MotherDairy, Plant_SahyadriAgro

    contracts::NodeId f_sonipat("Farm_Sonipat");
    contracts::NodeId f_nashik("Farm_Nashik");
    contracts::NodeId f_kolar("Farm_KolarRural");

    contracts::NodeId j_panipat("Jct_Panipat");
    contracts::NodeId j_nh44("Jct_NH44");
    contracts::NodeId j_thane("Jct_Thane");
    contracts::NodeId j_hosur("Jct_Hosur");

    contracts::NodeId m_azadpur("Mandi_Azadpur");
    contracts::NodeId m_ghazipur("Mandi_Ghazipur");
    contracts::NodeId m_vashi("Mandi_Vashi");
    contracts::NodeId m_kolar("Mandi_Kolar");

    contracts::NodeId p_motherdairy("Plant_MotherDairy");
    contracts::NodeId p_sahyadri("Plant_SahyadriAgro");

    // Northern cluster (Delhi NCR)
    graph.addBidirectionalEdge(f_sonipat, j_panipat, 25.0, contracts::SimTime(35), contracts::Money::fromRupees(50.0));
    graph.addBidirectionalEdge(j_panipat, m_azadpur, 45.0, contracts::SimTime(60), contracts::Money::fromRupees(120.0));
    graph.addBidirectionalEdge(j_panipat, p_motherdairy, 20.0, contracts::SimTime(30), contracts::Money::fromRupees(0.0));
    graph.addBidirectionalEdge(m_azadpur, m_ghazipur, 30.0, contracts::SimTime(50), contracts::Money::fromRupees(80.0));

    // Western cluster (Maharashtra)
    graph.addBidirectionalEdge(f_nashik, j_thane, 140.0, contracts::SimTime(180), contracts::Money::fromRupees(250.0));
    graph.addBidirectionalEdge(j_thane, m_vashi, 30.0, contracts::SimTime(45), contracts::Money::fromRupees(90.0));
    graph.addBidirectionalEdge(j_thane, p_sahyadri, 15.0, contracts::SimTime(25), contracts::Money::fromRupees(0.0));

    // Southern cluster (Karnataka)
    graph.addBidirectionalEdge(f_kolar, m_kolar, 20.0, contracts::SimTime(30), contracts::Money::fromRupees(30.0));
    graph.addBidirectionalEdge(m_kolar, j_hosur, 60.0, contracts::SimTime(75), contracts::Money::fromRupees(110.0));
    graph.addBidirectionalEdge(j_hosur, j_nh44, 40.0, contracts::SimTime(50), contracts::Money::fromRupees(60.0));

    // Cross-link corridors
    graph.addBidirectionalEdge(j_panipat, j_nh44, 180.0, contracts::SimTime(220), contracts::Money::fromRupees(300.0));
    graph.addBidirectionalEdge(j_nh44, m_azadpur, 50.0, contracts::SimTime(70), contracts::Money::fromRupees(100.0));

    return graph;
}

} // namespace kisansetu::domain
