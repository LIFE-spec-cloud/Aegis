#include "agents/LogisticsBroker.h"
#include "core/EventBus.h"

namespace kisansetu::agents {

// ─── Agent Registry ─────────────────────────────────────────────────────────

void LogisticsBroker::registerTransporter(const contracts::AgentId& id) {
    registeredTransporters_.push_back(id);
}

void LogisticsBroker::registerMandi(const contracts::AgentId& id, const contracts::NodeId& location) {
    registeredMandis_.push_back({id, location});
}

void LogisticsBroker::registerFarmer(const contracts::AgentId& id) {
    registeredFarmers_.push_back(id);
}

void LogisticsBroker::registerProcessor(const contracts::AgentId& id) {
    registeredProcessors_.push_back(id);
}

void LogisticsBroker::registerSlotPort(const contracts::AgentId& mandiId,
                                        SlotReserveFunc reserve,
                                        SlotCancelFunc cancel) {
    slotPorts_[mandiId] = SlotPort{std::move(reserve), std::move(cancel)};
}

// ─── Main Message Dispatcher (CNP Orchestration) ────────────────────────────

void LogisticsBroker::onMessage(const contracts::Envelope& envelope) {
    switch (envelope.performative()) {
        case contracts::Performative::TASK_ANNOUNCEMENT:
            handleTaskAnnouncement(envelope);
            break;
        case contracts::Performative::PROPOSE:
            handleProposal(envelope);
            break;
        case contracts::Performative::ACCEPT:
            handleFarmerAccept(envelope);
            break;
        case contracts::Performative::REJECT:
            handleFarmerReject(envelope);
            break;
        case contracts::Performative::FAILURE:
            handleFailure(envelope);
            break;
        default:
            // Log unhandled performatives for debugging
            eventLog_.log(envelope.sentAt(), "BROKER_UNHANDLED_MSG", id(), envelope.sender(),
                          envelope.correlationId(),
                          "Unhandled performative: " + contracts::toString(envelope.performative()));
            break;
    }
}

// ─── CNP Step 1: Receive Task Announcement, broadcast CFPs ──────────────────

void LogisticsBroker::handleTaskAnnouncement(const contracts::Envelope& envelope) {
    if (!std::holds_alternative<contracts::TaskAnnouncementPayload>(envelope.payload())) {
        return;
    }
    const auto& ann = std::get<contracts::TaskAnnouncementPayload>(envelope.payload());

    contracts::ContractId contractId(nextContractId_++);
    ContractSession session;
    session.contractId = contractId;
    session.farmerId = ann.farmerId;
    session.crop = ann.crop;
    session.volume = ann.volume;
    session.origin = ann.farmLocation;
    session.readyTime = ann.readyTime;
    session.deadline = ann.deadline;
    session.state = contracts::ContractState::CREATED;

    eventLog_.log(envelope.sentAt(), "CNP_SESSION_CREATED", id(), ann.farmerId, contractId,
                  "Crop=" + contracts::toString(ann.crop) +
                  " Vol=" + std::to_string(ann.volume.value()) + "qtl" +
                  " Reserve=" + std::to_string(ann.reservePricePerQuintal.toRupees()) + "INR/qtl");

    sessions_[contractId] = std::move(session);
    broadcastCfp(sessions_[contractId], envelope.sentAt());
}

void LogisticsBroker::broadcastCfp(ContractSession& session, contracts::SimTime sentAt) {
    // Broadcast CFP to all registered transporters
    for (const auto& tid : registeredTransporters_) {
        contracts::CfpPayload cfp{
            .contractId = session.contractId,
            .farmerId = session.farmerId,
            .crop = session.crop,
            .volume = session.volume,
            .origin = session.origin,
            .destination = (!registeredMandis_.empty()) ? registeredMandis_[0].location : contracts::NodeId("Unknown"),
            .readyTime = session.readyTime,
            .deadline = session.deadline
        };

        if (eventBus_) {
            eventBus_->send(contracts::Envelope(
                "CFP_TRUCK_" + std::to_string(session.contractId.value()) + "_" + tid.str(),
                session.contractId,
                id(),
                tid,
                contracts::Performative::CFP,
                cfp,
                sentAt,
                contracts::SimTime(60)
            ));
        }
    }

    // Broadcast CFP to all registered mandis
    for (const auto& minfo : registeredMandis_) {
        contracts::CfpPayload cfp{
            .contractId = session.contractId,
            .farmerId = session.farmerId,
            .crop = session.crop,
            .volume = session.volume,
            .origin = session.origin,
            .destination = minfo.location,
            .readyTime = session.readyTime,
            .deadline = session.deadline
        };

        if (eventBus_) {
            eventBus_->send(contracts::Envelope(
                "CFP_MANDI_" + std::to_string(session.contractId.value()) + "_" + minfo.id.str(),
                session.contractId,
                id(),
                minfo.id,
                contracts::Performative::CFP,
                cfp,
                sentAt,
                contracts::SimTime(60)
            ));
        }
    }

    eventLog_.log(sentAt, "CNP_CFP_BROADCAST", id(), contracts::AgentId("Broadcast"),
                  session.contractId,
                  "Transporters=" + std::to_string(registeredTransporters_.size()) +
                  " Mandis=" + std::to_string(registeredMandis_.size()));
}

// ─── CNP Step 2: Collect Proposals ──────────────────────────────────────────

void LogisticsBroker::handleProposal(const contracts::Envelope& envelope) {
    if (!std::holds_alternative<contracts::ProposePayload>(envelope.payload())) {
        return;
    }
    const auto& prop = std::get<contracts::ProposePayload>(envelope.payload());

    auto it = sessions_.find(prop.contractId);
    if (it == sessions_.end()) {
        return; // Unknown contract, discard
    }
    auto& session = it->second;
    if (session.state != contracts::ContractState::CREATED) {
        return; // Not accepting proposals for non-CREATED sessions
    }

    if (prop.role == contracts::BidderRole::Transporter) {
        session.transporterProposals.push_back(prop);
        eventLog_.log(envelope.sentAt(), "CNP_PROPOSAL_RECV_TRUCK", id(), prop.bidderId,
                      session.contractId,
                      "Freight=" + std::to_string(prop.costOrPrice.toRupees()) +
                      " ETA=" + std::to_string(prop.slotOrEta.ticks()) + "min" +
                      " Refrig=" + std::string(prop.isRefrigerated ? "Y" : "N"));
    } else if (prop.role == contracts::BidderRole::Mandi) {
        session.mandiProposals.push_back(prop);
        eventLog_.log(envelope.sentAt(), "CNP_PROPOSAL_RECV_MANDI", id(), prop.bidderId,
                      session.contractId,
                      "Price=" + std::to_string(prop.costOrPrice.toRupees()) +
                      " Slot=" + std::to_string(prop.slotOrEta.ticks()) + "min");
    }

    // Forward proposal to farmer for ranking
    if (eventBus_) {
        eventBus_->send(contracts::Envelope(
            "FWD_PROP_" + std::to_string(session.contractId.value()) + "_" + prop.bidderId.str(),
            session.contractId,
            id(),
            session.farmerId,
            contracts::Performative::PROPOSE,
            prop,
            envelope.sentAt(),
            contracts::SimTime(30)
        ));
    }
}

// ─── CNP Step 3: Two-Phase Commit (Farmer ACCEPT) ──────────────────────────

void LogisticsBroker::handleFarmerAccept(const contracts::Envelope& envelope) {
    if (!std::holds_alternative<contracts::AcceptPayload>(envelope.payload())) {
        return;
    }
    const auto& accept = std::get<contracts::AcceptPayload>(envelope.payload());

    auto it = sessions_.find(accept.contractId);
    if (it == sessions_.end()) {
        return;
    }
    auto& session = it->second;

    eventLog_.log(envelope.sentAt(), "CNP_FARMER_AWARD_RECEIVED", id(), accept.farmerId,
                  accept.contractId,
                  "Truck=" + accept.transporterId.str() +
                  " Mandi=" + accept.mandiId.str() +
                  " Freight=" + std::to_string(accept.agreedFreight.toRupees()));

    session.state = contracts::ContractState::PREPARING;
    session.awardedTransporterId = accept.transporterId;
    session.awardedMandiId = accept.mandiId;

    // Execute two-phase commit: lock truck, reserve slot, hold escrow
    if (executeCommit(session, accept, envelope.sentAt())) {
        session.state = contracts::ContractState::COMMITTED;

        eventLog_.log(envelope.sentAt(), "CNP_TWO_PHASE_COMMITTED", id(), accept.farmerId,
                      accept.contractId, "All resources locked and committed");

        // Notify farmer of commit
        if (eventBus_) {
            contracts::InformPayload commitMsg{.message = "Contract committed"};
            eventBus_->send(contracts::Envelope(
                "COMMIT_" + std::to_string(accept.contractId.value()),
                accept.contractId,
                id(),
                accept.farmerId,
                contracts::Performative::ACCEPT,
                commitMsg,
                envelope.sentAt(),
                contracts::SimTime(30)
            ));

            // Notify awarded transporter
            eventBus_->send(contracts::Envelope(
                "DISPATCH_" + std::to_string(accept.contractId.value()) + "_" + accept.transporterId.str(),
                accept.contractId,
                id(),
                accept.transporterId,
                contracts::Performative::ACCEPT,
                accept,
                envelope.sentAt(),
                contracts::SimTime(30)
            ));
        }

        // Reject all non-awarded transporters
        for (const auto& prop : session.transporterProposals) {
            if (!(prop.bidderId == accept.transporterId)) {
                if (eventBus_) {
                    contracts::RejectPayload rej{
                        .contractId = accept.contractId,
                        .rejectingAgentId = id(),
                        .reason = "Another transporter was selected"
                    };
                    eventBus_->send(contracts::Envelope(
                        "REJ_TRUCK_" + prop.bidderId.str(),
                        accept.contractId,
                        id(),
                        prop.bidderId,
                        contracts::Performative::REJECT,
                        rej,
                        envelope.sentAt(),
                        contracts::SimTime(30)
                    ));
                }
            }
        }

        // Reject all non-awarded mandis
        for (const auto& prop : session.mandiProposals) {
            if (!(prop.bidderId == accept.mandiId)) {
                if (eventBus_) {
                    contracts::RejectPayload rej{
                        .contractId = accept.contractId,
                        .rejectingAgentId = id(),
                        .reason = "Another mandi was selected"
                    };
                    eventBus_->send(contracts::Envelope(
                        "REJ_MANDI_" + prop.bidderId.str(),
                        accept.contractId,
                        id(),
                        prop.bidderId,
                        contracts::Performative::REJECT,
                        rej,
                        envelope.sentAt(),
                        contracts::SimTime(30)
                    ));
                }
            }
        }
    } else {
        rollbackSession(session, envelope.sentAt(), "Two-phase commit failed: resource acquisition failure");
    }
}

bool LogisticsBroker::executeCommit(ContractSession& session,
                                     const contracts::AcceptPayload& accept,
                                     contracts::SimTime sentAt) {
    // Phase 1: Lock Truck
    auto truckLock = lockTruck(accept.transporterId, accept.contractId);
    if (!truckLock.has_value()) {
        eventLog_.log(sentAt, "COMMIT_FAIL_TRUCK_LOCK", id(), accept.transporterId,
                      accept.contractId, "Truck already locked by another contract");
        return false;
    }
    session.truckHandle = std::move(truckLock);

    // Phase 2: Reserve Gate Slot
    auto slotPortIt = slotPorts_.find(accept.mandiId);
    if (slotPortIt != slotPorts_.end()) {
        auto slotHandle = slotPortIt->second.reserve(accept.mandiId, accept.contractId, accept.scheduledGateSlot);
        if (!slotHandle.has_value()) {
            eventLog_.log(sentAt, "COMMIT_FAIL_SLOT_RESERVE", id(), accept.mandiId,
                          accept.contractId, "Gate slot unavailable (quota full)");
            // Rollback truck lock
            if (session.truckHandle.has_value()) {
                releaseTruck(*session.truckHandle);
                session.truckHandle.reset();
            }
            return false;
        }
        session.slotHandle = std::move(slotHandle);
    }

    // Phase 3: Hold Escrow Deposit
    contracts::Money deposit = accept.agreedFreight * 0.20; // 20% deposit
    auto escrow = holdDeposit(accept.farmerId, accept.contractId, deposit);
    if (!escrow.has_value()) {
        eventLog_.log(sentAt, "COMMIT_FAIL_ESCROW", id(), accept.farmerId,
                      accept.contractId, "Escrow deposit failed");
        // Rollback slot
        if (session.slotHandle.has_value()) {
            auto cancelIt = slotPorts_.find(accept.mandiId);
            if (cancelIt != slotPorts_.end()) {
                cancelIt->second.cancel(*session.slotHandle);
            }
            session.slotHandle.reset();
        }
        // Rollback truck lock
        if (session.truckHandle.has_value()) {
            releaseTruck(*session.truckHandle);
            session.truckHandle.reset();
        }
        return false;
    }
    session.escrowHandle = std::move(escrow);

    return true;
}

void LogisticsBroker::rollbackSession(ContractSession& session, contracts::SimTime sentAt,
                                       const std::string& reason) {
    session.state = contracts::ContractState::ABORTED;
    eventLog_.log(sentAt, "CNP_SESSION_ABORTED", id(), session.farmerId,
                  session.contractId, reason);

    // Release any acquired resources
    if (session.escrowHandle.has_value()) {
        refundDeposit(*session.escrowHandle);
        session.escrowHandle.reset();
    }
    if (session.slotHandle.has_value()) {
        auto cancelIt = slotPorts_.find(session.awardedMandiId);
        if (cancelIt != slotPorts_.end()) {
            cancelIt->second.cancel(*session.slotHandle);
        }
        session.slotHandle.reset();
    }
    if (session.truckHandle.has_value()) {
        releaseTruck(*session.truckHandle);
        session.truckHandle.reset();
    }

    // Notify farmer
    if (eventBus_) {
        contracts::RejectPayload rej{
            .contractId = session.contractId,
            .rejectingAgentId = id(),
            .reason = reason
        };
        eventBus_->send(contracts::Envelope(
            "BROKER_REJECT_" + std::to_string(session.contractId.value()),
            session.contractId,
            id(),
            session.farmerId,
            contracts::Performative::REJECT,
            rej,
            sentAt,
            contracts::SimTime(30)
        ));
    }
}

// ─── CNP: Farmer Reject ─────────────────────────────────────────────────────

void LogisticsBroker::handleFarmerReject(const contracts::Envelope& envelope) {
    if (!std::holds_alternative<contracts::RejectPayload>(envelope.payload())) {
        return;
    }
    const auto& rej = std::get<contracts::RejectPayload>(envelope.payload());

    auto it = sessions_.find(rej.contractId);
    if (it == sessions_.end()) {
        return;
    }
    auto& session = it->second;

    eventLog_.log(envelope.sentAt(), "CNP_FARMER_REJECTED", id(), rej.rejectingAgentId,
                  rej.contractId, "Reason: " + rej.reason);

    session.state = contracts::ContractState::ABORTED;
}

// ─── Failure Handling ───────────────────────────────────────────────────────

void LogisticsBroker::handleFailure(const contracts::Envelope& envelope) {
    if (!std::holds_alternative<contracts::FailureNoticePayload>(envelope.payload())) {
        return;
    }
    const auto& notice = std::get<contracts::FailureNoticePayload>(envelope.payload());

    auto it = sessions_.find(notice.contractId);
    if (it == sessions_.end()) {
        return;
    }
    auto& session = it->second;

    eventLog_.log(envelope.sentAt(), "CNP_FAILURE_RECEIVED", id(), notice.affectedAgentId,
                  notice.contractId,
                  "Type=" + notice.failureType + " Details=" + notice.details);

    // Mark session as compensating
    session.state = contracts::ContractState::COMPENSATED;
}

// ─── Session Accessor ───────────────────────────────────────────────────────

const ContractSession* LogisticsBroker::findSession(contracts::ContractId id) const {
    auto it = sessions_.find(id);
    if (it != sessions_.end()) {
        return &(it->second);
    }
    return nullptr;
}

// ─── ITruckLockPort ─────────────────────────────────────────────────────────

std::optional<contracts::TruckLockHandle> LogisticsBroker::lockTruck(
    const contracts::AgentId& truckId,
    const contracts::ContractId& contractId) {
    if (lockedTrucks_.contains(truckId)) {
        return std::nullopt; // Already locked
    }
    lockedTrucks_.insert(truckId);
    eventLog_.log(contracts::SimTime(0), "TRUCK_LOCKED", id(), truckId, contractId, "Truck locked");
    return contracts::TruckLockHandle(truckId, contractId, ++nextTruckLockId_);
}

bool LogisticsBroker::releaseTruck(contracts::TruckLockHandle& handle) {
    if (!handle.isValid()) {
        return false;
    }
    eventLog_.log(contracts::SimTime(0), "TRUCK_RELEASED", id(), handle.truckId(), handle.contractId(), "Truck released");
    lockedTrucks_.erase(handle.truckId());
    handle.invalidate();
    return true;
}

bool LogisticsBroker::rebindTruck(
    contracts::TruckLockHandle& handle,
    const contracts::AgentId& newTruckId) {
    if (!handle.isValid()) {
        return false;
    }
    if (lockedTrucks_.contains(newTruckId)) {
        return false; // Replacement truck is already locked
    }
    eventLog_.log(contracts::SimTime(0), "TRUCK_REBOUND", handle.truckId(), newTruckId, handle.contractId(), "Truck rebound");
    lockedTrucks_.erase(handle.truckId());
    lockedTrucks_.insert(newTruckId);
    handle.rebind(newTruckId, ++nextTruckLockId_);
    return true;
}

// ─── IEscrowPort ────────────────────────────────────────────────────────────

std::optional<contracts::EscrowHandle> LogisticsBroker::holdDeposit(
    const contracts::AgentId& farmerId,
    const contracts::ContractId& contractId,
    contracts::Money amount) {
    escrowLedger_[contractId] = amount;
    eventLog_.log(contracts::SimTime(0), "ESCROW_HELD", farmerId, id(), contractId, "Escrow deposit held: " + std::to_string(amount.toRupees()) + " INR");
    return contracts::EscrowHandle(farmerId, contractId, amount, ++nextEscrowId_);
}

bool LogisticsBroker::refundDeposit(contracts::EscrowHandle& handle) {
    if (!handle.isValid()) {
        return false;
    }
    eventLog_.log(contracts::SimTime(0), "ESCROW_REFUNDED", id(), handle.farmerId(), handle.contractId(), "Escrow deposit refunded");
    escrowLedger_.erase(handle.contractId());
    handle.invalidate();
    return true;
}

bool LogisticsBroker::transferDeposit(
    contracts::EscrowHandle& handle,
    const contracts::AgentId& recipientId,
    contracts::Money /*transferAmount*/) {
    if (!handle.isValid()) {
        return false;
    }
    eventLog_.log(contracts::SimTime(0), "ESCROW_TRANSFERRED", id(), recipientId, handle.contractId(), "Escrow deposit transferred");
    escrowLedger_.erase(handle.contractId());
    handle.invalidate();
    return true;
}

// ─── IRecoveryContext ───────────────────────────────────────────────────────

std::vector<contracts::AgentId> LogisticsBroker::findIdleTrucks(
    const contracts::NodeId& /*location*/,
    contracts::Quintals /*capacity*/) const {
    // Return transporters not currently locked
    std::vector<contracts::AgentId> idle;
    for (const auto& tid : registeredTransporters_) {
        if (!lockedTrucks_.contains(tid)) {
            idle.push_back(tid);
        }
    }
    return idle;
}

std::vector<contracts::MandiCandidate> LogisticsBroker::rankMandis(
    const contracts::CropBatchInfo& /*batch*/,
    const std::vector<contracts::AgentId>& /*exclude*/) const {
    return {};
}

std::vector<contracts::AgentId> LogisticsBroker::findProcessors(
    const contracts::NodeId& /*location*/,
    double /*radiusKm*/) const {
    return registeredProcessors_;
}

contracts::RouteInfo LogisticsBroker::routeTo(
    const contracts::NodeId& from,
    const contracts::NodeId& to,
    contracts::RouteStrategyType /*strategy*/) const {
    return contracts::RouteInfo{
        .path = {from, to},
        .distanceKm = 50.0,
        .estimatedTransitTime = contracts::SimTime(60),
        .tollCost = contracts::Money(0),
        .freightCost = contracts::Money::fromRupees(1500.0)
    };
}

double LogisticsBroker::predictQuality(
    const contracts::CropBatchInfo& /*batch*/,
    const contracts::RouteInfo& /*route*/,
    double /*ambientTempC*/) const {
    return 0.80;
}

} // namespace kisansetu::agents
