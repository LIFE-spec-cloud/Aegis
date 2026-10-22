#include "agents/FarmerAgent.h"
#include "contracts/Payloads.h"
#include "domain/PriceModel.h"
#include <limits>

namespace kisansetu::agents {

FarmerAgent::FarmerAgent(contracts::AgentId id,
                         contracts::Money reservePrice,
                         contracts::Money depositBudget,
                         double riskAversion)
    : core::Agent(std::move(id)),
      reservePrice_(reservePrice),
      depositBudget_(depositBudget),
      riskAversion_(riskAversion) {}

void FarmerAgent::setDependencies(core::EventBus* bus,
                                  contracts::EventLog* log,
                                  contracts::AgentId brokerId) {
    eventBus_ = bus;
    eventLog_ = log;
    brokerId_ = std::move(brokerId);
}

void FarmerAgent::announceHarvest(domain::CropBatch batch,
                                  contracts::NodeId location,
                                  contracts::SimTime readyTime,
                                  contracts::SimTime deadline,
                                  contracts::SimTime currentTime) {
    currentBatch_ = std::move(batch);
    farmLocation_ = std::move(location);
    readyTime_ = readyTime;
    deadline_ = deadline;
    contractState_ = contracts::ContractState::CREATED;

    transporterProposals_.clear();
    mandiProposals_.clear();

    contracts::TaskAnnouncementPayload announcement{
        .farmerId = id(),
        .crop = currentBatch_->commodity().type,
        .volume = currentBatch_->volume(),
        .readyTime = readyTime_,
        .deadline = deadline_,
        .reservePricePerQuintal = reservePrice_,
        .farmLocation = farmLocation_
    };

    if (eventLog_) {
        eventLog_->log(currentTime, "TASK_ANNOUNCED", id(), brokerId_, contracts::ContractId(0),
                       "Crop=" + currentBatch_->commodity().name() +
                       " Vol=" + std::to_string(currentBatch_->volume().value()) + "qtl" +
                       " Reserve=" + std::to_string(reservePrice_.toRupees()) + " INR/qtl");
    }

    if (eventBus_) {
        eventBus_->send(contracts::Envelope(
            "TASK_ANN_" + id().str(),
            contracts::ContractId(0),
            id(),
            brokerId_,
            contracts::Performative::TASK_ANNOUNCEMENT,
            announcement,
            currentTime,
            contracts::SimTime(60)
        ));
    }
}

void FarmerAgent::handleOrderCommand(const contracts::DomainCommand& command) {
    if (std::holds_alternative<contracts::CreateHarvestBatchCommand>(command)) {
        const auto& cmd = std::get<contracts::CreateHarvestBatchCommand>(command);
        domain::Commodity comm = domain::Commodity::fromType(cmd.crop);
        domain::CropBatch batch("BATCH-" + id().str(), comm, cmd.quantity, cmd.readyTime);
        announceHarvest(std::move(batch), contracts::NodeId("Farm_" + id().str()),
                        cmd.readyTime, cmd.deadline, cmd.readyTime);
    }
}

void FarmerAgent::onMessage(const contracts::Envelope& envelope) {
    // 1. Collect Proposals
    if (envelope.performative() == contracts::Performative::PROPOSE) {
        if (!std::holds_alternative<contracts::ProposePayload>(envelope.payload())) {
            return;
        }
        const auto& prop = std::get<contracts::ProposePayload>(envelope.payload());
        activeContractId_ = envelope.correlationId();

        if (prop.role == contracts::BidderRole::Transporter) {
            transporterProposals_.push_back(prop);
        } else if (prop.role == contracts::BidderRole::Mandi) {
            mandiProposals_.push_back(prop);
        }

        // When we have at least one transporter and one mandi proposal, evaluate bundles
        if (!transporterProposals_.empty() && !mandiProposals_.empty() &&
            contractState_ == contracts::ContractState::CREATED) {
            evaluateAndAwardProposals(envelope.sentAt());
        }
    }
    // 2. Broker confirms commit
    else if (envelope.performative() == contracts::Performative::ACCEPT) {
        contractState_ = contracts::ContractState::COMMITTED;
        if (eventLog_) {
            eventLog_->log(envelope.sentAt(), "FARMER_CONTRACT_COMMITTED", id(), envelope.sender(),
                           envelope.correlationId(), "Two-phase commit confirmed by broker");
        }
    }
    // 3. Broker notifies rejection (e.g. lost slot race or budget breach)
    else if (envelope.performative() == contracts::Performative::REJECT) {
        contractState_ = contracts::ContractState::ABORTED;
        if (eventLog_) {
            eventLog_->log(envelope.sentAt(), "FARMER_CONTRACT_REJECTED", id(), envelope.sender(),
                           envelope.correlationId(), "Contract negotiation aborted / lost race");
        }
    }
}

void FarmerAgent::evaluateAndAwardProposals(contracts::SimTime currentTime) {
    if (!currentBatch_.has_value() || transporterProposals_.empty() || mandiProposals_.empty()) {
        return;
    }

    contracts::Money bestNetMargin(-std::numeric_limits<int64_t>::max() / 2);
    const contracts::ProposePayload* bestTruck = nullptr;
    const contracts::ProposePayload* bestMandi = nullptr;

    double volumeQtl = currentBatch_->volume().value();
    contracts::Money mandiFees = contracts::Money::fromRupees(50.0 * volumeQtl);

    for (const auto& truck : transporterProposals_) {
        for (const auto& mandi : mandiProposals_) {
            // Predict quality upon arrival at mandi
            double predictedQ = currentBatch_->predictQualityAt(truck.slotOrEta, 28.0, truck.isRefrigerated);
            auto state = domain::CropFreshnessState::fromQuality(predictedQ);
            double gradeMultiplier = state->priceMultiplier();

            contracts::Money net = domain::PriceModel::calculateNetMargin(
                mandi.costOrPrice,
                gradeMultiplier,
                currentBatch_->volume(),
                truck.costOrPrice,
                mandiFees
            );

            // Risk-adjusted discount based on ETA distance and risk aversion
            int64_t delayVarianceTicks = truck.slotOrEta.ticks() - readyTime_.ticks();
            int64_t riskDiscountPaise = static_cast<int64_t>(riskAversion_ * delayVarianceTicks * 100);
            contracts::Money riskAdjustedNet = net - contracts::Money::fromPaise(riskDiscountPaise);

            if (riskAdjustedNet > bestNetMargin) {
                bestNetMargin = riskAdjustedNet;
                bestTruck = &truck;
                bestMandi = &mandi;
            }
        }
    }

    if (bestTruck && bestMandi) {
        contracts::Money netPerQuintal = bestNetMargin / volumeQtl;
        if (netPerQuintal >= reservePrice_) {
            // Award contract bundle
            contractState_ = contracts::ContractState::PREPARING;
            agreedNetMargin_ = bestNetMargin;

            contracts::AcceptPayload acceptPayload{
                .contractId = activeContractId_,
                .farmerId = id(),
                .transporterId = bestTruck->bidderId,
                .mandiId = bestMandi->bidderId,
                .agreedFreight = bestTruck->costOrPrice,
                .expectedMandiPrice = bestMandi->costOrPrice,
                .scheduledGateSlot = bestMandi->slotOrEta,
                .expectedArrival = bestTruck->slotOrEta
            };

            if (eventLog_) {
                eventLog_->log(currentTime, "FARMER_AWARD_BUNDLE", id(), brokerId_, activeContractId_,
                               "Truck=" + bestTruck->bidderId.str() +
                               " Mandi=" + bestMandi->bidderId.str() +
                               " NetMargin=" + std::to_string(bestNetMargin.toRupees()) + " INR");
            }

            if (eventBus_) {
                eventBus_->send(contracts::Envelope(
                    "AWARD_" + id().str() + "_" + std::to_string(activeContractId_.value()),
                    activeContractId_,
                    id(),
                    brokerId_,
                    contracts::Performative::ACCEPT,
                    acceptPayload,
                    currentTime,
                    contracts::SimTime(30)
                ));
            }
            return;
        }
    }

    // Did not meet reserve price
    contractState_ = contracts::ContractState::ABORTED;
    if (eventBus_) {
        contracts::RejectPayload rejectPayload{
            .contractId = activeContractId_,
            .rejectingAgentId = id(),
            .reason = "Net margin below farmer reserve price"
        };
        eventBus_->send(contracts::Envelope(
            "REJECT_" + id().str(),
            activeContractId_,
            id(),
            brokerId_,
            contracts::Performative::REJECT,
            rejectPayload,
            currentTime,
            contracts::SimTime(30)
        ));
    }
}

} // namespace kisansetu::agents
