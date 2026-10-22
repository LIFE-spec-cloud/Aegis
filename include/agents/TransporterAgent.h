#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include "contracts/ITelemetry.h"
#include "contracts/EventLog.h"
#include "core/EventBus.h"
#include "domain/RoadGraph.h"
#include "strategy/IRoutePlanningStrategy.h"
#include <memory>
#include <optional>

namespace kisansetu::agents {

/**
 * @brief Autonomous Transporter Agent.
 *
 * Holds private state: cost/km, mechanical health, position, true ETA, state.
 * Emits periodic telemetry and drives the road graph.
 */
class TransporterAgent : public core::Agent {
public:
    TransporterAgent(contracts::AgentId id,
                     contracts::NodeId baseLocation,
                     contracts::Quintals capacity,
                     contracts::Money costPerKm,
                     bool isRefrigerated);

    void setDependencies(core::EventBus* bus,
                         contracts::EventLog* log,
                         domain::RoadGraph* graph,
                         std::unique_ptr<strategy::IRoutePlanningStrategy> routeStrategy = nullptr);

    void onMessage(const contracts::Envelope& envelope) override;

    [[nodiscard]] const contracts::NodeId& currentLocation() const noexcept { return currentLocation_; }
    [[nodiscard]] contracts::Quintals capacity() const noexcept { return capacity_; }
    [[nodiscard]] contracts::Money costPerKm() const noexcept { return costPerKm_; }
    [[nodiscard]] bool isRefrigerated() const noexcept { return isRefrigerated_; }
    [[nodiscard]] contracts::TruckState state() const noexcept { return state_; }
    [[nodiscard]] contracts::ContractId activeContractId() const noexcept { return activeContractId_; }

    void injectStall(contracts::SimTime time);
    void injectFailure(contracts::SimTime time);
    void emitTelemetry(contracts::SimTime time, double currentQuality = 1.0, std::string batchId = "");

private:
    contracts::NodeId currentLocation_;
    contracts::Quintals capacity_{0.0};
    contracts::Money costPerKm_{0};
    bool isRefrigerated_{false};
    contracts::TruckState state_{contracts::TruckState::IDLE};
    double speedKmh_{45.0};
    double mechanicalHealth_{1.0};

    core::EventBus* eventBus_{nullptr};
    contracts::EventLog* eventLog_{nullptr};
    domain::RoadGraph* roadGraph_{nullptr};
    std::unique_ptr<strategy::IRoutePlanningStrategy> routeStrategy_;

    contracts::ContractId activeContractId_{0};
    std::optional<contracts::RouteInfo> currentRoute_;
    contracts::SimTime expectedArrival_{0};
};

} // namespace kisansetu::agents
