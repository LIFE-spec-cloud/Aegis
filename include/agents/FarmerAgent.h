#pragma once

#include "core/Agent.h"
#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include "contracts/IOrderInputPort.h"
#include "contracts/EventLog.h"
#include "core/EventBus.h"
#include "domain/CropBatch.h"
#include <vector>
#include <optional>

namespace kisansetu::agents {

/**
 * @brief Autonomous Farmer Agent.
 *
 * Holds private state: reserve price, deposit budget, risk aversion, crop batches.
 * Evaluates candidate bundles (transporter, mandi) by risk-adjusted net margin.
 */
class FarmerAgent : public core::Agent {
public:
    FarmerAgent(contracts::AgentId id,
                contracts::Money reservePrice,
                contracts::Money depositBudget,
                double riskAversion);

    void setDependencies(core::EventBus* bus,
                         contracts::EventLog* log,
                         contracts::AgentId brokerId);

    void onMessage(const contracts::Envelope& envelope) override;

    void announceHarvest(domain::CropBatch batch,
                         contracts::NodeId location,
                         contracts::SimTime readyTime,
                         contracts::SimTime deadline,
                         contracts::SimTime currentTime);

    void handleOrderCommand(const contracts::DomainCommand& command);

    void evaluateAndAwardProposals(contracts::SimTime currentTime);

    [[nodiscard]] contracts::Money reservePrice() const noexcept { return reservePrice_; }
    [[nodiscard]] contracts::Money depositBudget() const noexcept { return depositBudget_; }
    [[nodiscard]] double riskAversion() const noexcept { return riskAversion_; }
    [[nodiscard]] contracts::ContractState contractState() const noexcept { return contractState_; }
    [[nodiscard]] contracts::ContractId activeContractId() const noexcept { return activeContractId_; }
    [[nodiscard]] contracts::Money agreedNetMargin() const noexcept { return agreedNetMargin_; }

private:
    contracts::Money reservePrice_{0};
    contracts::Money depositBudget_{0};
    double riskAversion_{0.1};

    core::EventBus* eventBus_{nullptr};
    contracts::EventLog* eventLog_{nullptr};
    contracts::AgentId brokerId_{"Broker"};

    std::optional<domain::CropBatch> currentBatch_;
    contracts::NodeId farmLocation_;
    contracts::SimTime readyTime_{0};
    contracts::SimTime deadline_{0};

    contracts::ContractId activeContractId_{0};
    contracts::ContractState contractState_{contracts::ContractState::CREATED};
    contracts::Money agreedNetMargin_{0};

    std::vector<contracts::ProposePayload> transporterProposals_;
    std::vector<contracts::ProposePayload> mandiProposals_;
};

} // namespace kisansetu::agents
