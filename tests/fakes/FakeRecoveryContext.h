#pragma once

#include "../../include/contracts/IRecoveryContext.h"
#include <cmath>

namespace agro::fakes {

class FakeRecoveryContext : public IRecoveryContext {
public:
    void addTruck(CandidateTruck truck) {
        m_trucks.push_back(std::move(truck));
    }

    void addMandi(CandidateMandi mandi) {
        m_mandis.push_back(std::move(mandi));
    }

    void addProcessor(CandidateProcessor processor) {
        m_processors.push_back(std::move(processor));
    }

    void clear() {
        m_trucks.clear();
        m_mandis.clear();
        m_processors.clear();
    }

    [[nodiscard]] std::vector<CandidateTruck> findIdleTrucks(
        const std::string& locationNode,
        Quintals requiredCapacity) const override {
        
        std::vector<CandidateTruck> matches;
        for (const auto& truck : m_trucks) {
            // In fake, if locationNode is "*" or matches truck location and capacity >= required
            if ((locationNode.empty() || locationNode == "*" || truck.currentNode == locationNode) &&
                truck.capacity.value >= requiredCapacity.value) {
                matches.push_back(truck);
            }
        }
        return matches;
    }

    [[nodiscard]] std::vector<CandidateMandi> rankMandis(
        const CropBatch& /*batch*/,
        const std::string& /*fromNode*/,
        const std::vector<AgentId>& excludeMandis) const override {
        
        std::vector<CandidateMandi> candidates;
        for (const auto& mandi : m_mandis) {
            bool excluded = false;
            for (const auto& ex : excludeMandis) {
                if (mandi.mandiId == ex) {
                    excluded = true;
                    break;
                }
            }
            if (!excluded) {
                candidates.push_back(mandi);
            }
        }
        return candidates;
    }

    [[nodiscard]] std::vector<CandidateProcessor> findProcessors(
        const std::string& /*locationNode*/,
        double radiusKm) const override {
        
        std::vector<CandidateProcessor> candidates;
        for (const auto& proc : m_processors) {
            if (proc.distanceKm <= radiusKm) {
                candidates.push_back(proc);
            }
        }
        return candidates;
    }

    [[nodiscard]] RouteInfo routeTo(
        const std::string& fromNode,
        const std::string& toNode) const override {
        
        RouteInfo info;
        info.pathNodes = {fromNode, "Junction_1", toNode};
        info.totalDistanceKm = 120.0;
        info.estimatedDuration = SimTime::fromHours(2.5);
        info.tollAndFuelCost = Money::fromRupees(1500);
        return info;
    }

    [[nodiscard]] double predictQuality(
        const CropBatch& batch,
        SimTime transitTime) const override {
        // Q(t) = Q0 * exp(-lambda * t_hours * mu_temp)
        double hours = transitTime.toHours();
        double effectiveDecay = batch.decayLambda * batch.tempMultiplier * (batch.isRefrigerated ? 0.6 : 1.0);
        return batch.qualityQ0 * std::exp(-effectiveDecay * hours);
    }

private:
    std::vector<CandidateTruck> m_trucks;
    std::vector<CandidateMandi> m_mandis;
    std::vector<CandidateProcessor> m_processors;
};

} // namespace agro::fakes
