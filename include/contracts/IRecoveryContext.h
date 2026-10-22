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
    Money quotedPrice{0};
    Money expectedNetMargin{0};
    SimTime nextSlotTime{0};
    double distanceKm{0.0};
    SimTime estimatedTransitTime{0};
    double predictedArrivalQuality{1.0};
};

struct CandidateProcessor {
    AgentId processorId;
    std::string name;
    std::string locationNode;
    Money priceFloorPerQuintal{0};
    Money currentBidPricePerQuintal{0};
    double distanceKm{0.0};
    SimTime estimatedTransitTime{0};
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
        Quintals capacity) const {
        auto cands = findIdleTrucks(location.str(), capacity);
        std::vector<AgentId> res;
        for (const auto& c : cands) res.push_back(c.truckId);
        return res;
    }

    /**
     * @brief Rank available mandis for rerouting by risk-adjusted net margin.
     */
    [[nodiscard]] virtual std::vector<MandiCandidate> rankMandis(
        const CropBatchInfo& batch,
        const std::vector<AgentId>& exclude) const {
        agro::CropBatch b;
        b.commodity = batch.crop;
        b.volume = batch.volume;
        b.harvestTime = batch.harvestTime;
        b.qualityQ0 = batch.initialQuality;
        b.isRefrigerated = batch.isRefrigerated;
        auto cands = rankMandis(b, "Farm_Sonipat", exclude);
        std::vector<MandiCandidate> res;
        for (const auto& c : cands) {
            res.push_back(MandiCandidate{
                .mandiId = c.mandiId,
                .location = NodeId(c.name),
                .forecastPrice = c.quotedPrice,
                .netMargin = c.expectedNetMargin,
                .eta = c.estimatedTransitTime,
                .predictedArrivalQuality = c.predictedArrivalQuality
            });
        }
        return res;
    }

    /**
     * @brief Locate food processing plants within the given radius from a location.
     */
    [[nodiscard]] virtual std::vector<AgentId> findProcessors(
        const NodeId& location,
        double radiusKm) const {
        auto cands = findProcessors(location.str(), radiusKm);
        std::vector<AgentId> res;
        for (const auto& c : cands) res.push_back(c.processorId);
        return res;
    }

    /**
     * @brief Compute the optimal path between two nodes using the selected strategy.
     */
    [[nodiscard]] virtual RouteInfo routeTo(
        const NodeId& from,
        const NodeId& to,
        RouteStrategyType strategy = RouteStrategyType::DIJKSTRA) const {
        (void)strategy;
        return routeTo(from.str(), to.str());
    }

    /**
     * @brief Predict the arrival quality Q of a batch along a specified route.
     */
    [[nodiscard]] virtual double predictQuality(
        const CropBatchInfo& batch,
        const RouteInfo& route,
        double ambientTempC = 28.0) const {
        (void)ambientTempC;
        agro::CropBatch b;
        b.commodity = batch.crop;
        b.volume = batch.volume;
        b.harvestTime = batch.harvestTime;
        b.qualityQ0 = batch.initialQuality;
        b.isRefrigerated = batch.isRefrigerated;
        return predictQuality(b, route.estimatedTransitTime);
    }

    // Compatibility adapter methods for Person B recovery strategies
    [[nodiscard]] virtual std::vector<CandidateTruck> findIdleTrucks(
        const std::string& locationNode,
        Quintals requiredCapacity) const {
        auto truckIds = findIdleTrucks(NodeId(locationNode), requiredCapacity);
        std::vector<CandidateTruck> result;
        for (const auto& tid : truckIds) {
            result.push_back(CandidateTruck{tid, locationNode, requiredCapacity, SimTime(0), 12.0});
        }
        return result;
    }

    [[nodiscard]] virtual std::vector<CandidateMandi> rankMandis(
        const agro::CropBatch& batch,
        const std::string& fromNode,
        const std::vector<AgentId>& excludeMandis) const {
        (void)fromNode;
        CropBatchInfo binfo{
            .batchId = "BATCH-REC",
            .crop = batch.commodity,
            .volume = batch.volume,
            .initialQuality = batch.qualityQ0,
            .harvestTime = batch.harvestTime,
            .isRefrigerated = batch.isRefrigerated
        };
        auto mCands = rankMandis(binfo, excludeMandis);
        std::vector<CandidateMandi> result;
        for (const auto& c : mCands) {
            result.push_back(CandidateMandi{
                .mandiId = c.mandiId,
                .name = c.mandiId.str(),
                .quotedPrice = c.forecastPrice,
                .expectedNetMargin = c.netMargin,
                .nextSlotTime = c.eta,
                .distanceKm = 40.0,
                .estimatedTransitTime = c.eta,
                .predictedArrivalQuality = c.predictedArrivalQuality
            });
        }
        return result;
    }

    [[nodiscard]] virtual std::vector<CandidateProcessor> findProcessors(
        const std::string& locationNode,
        double radiusKm) const {
        auto pIds = findProcessors(NodeId(locationNode), radiusKm);
        std::vector<CandidateProcessor> result;
        for (const auto& pid : pIds) {
            result.push_back(CandidateProcessor{
                .processorId = pid,
                .name = pid.str(),
                .locationNode = locationNode,
                .priceFloorPerQuintal = Money::fromRupees(1500.0),
                .currentBidPricePerQuintal = Money::fromRupees(1850.0),
                .distanceKm = 15.0,
                .estimatedTransitTime = SimTime(30),
                .predictedArrivalQuality = 0.55
            });
        }
        return result;
    }

    [[nodiscard]] virtual RouteInfo routeTo(
        const std::string& fromNode,
        const std::string& toNode) const {
        return routeTo(NodeId(fromNode), NodeId(toNode), RouteStrategyType::DIJKSTRA);
    }

    [[nodiscard]] virtual double predictQuality(
        const agro::CropBatch& batch,
        SimTime transitTime) const {
        CropBatchInfo binfo{
            .batchId = "BATCH-REC",
            .crop = batch.commodity,
            .volume = batch.volume,
            .initialQuality = batch.qualityQ0,
            .harvestTime = batch.harvestTime,
            .isRefrigerated = batch.isRefrigerated
        };
        RouteInfo r;
        r.estimatedTransitTime = transitTime;
        return predictQuality(binfo, r, 28.0);
    }
};

} // namespace kisansetu::contracts
