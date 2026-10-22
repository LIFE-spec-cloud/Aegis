#include "agents/TransporterAgent.h"
#include "contracts/Payloads.h"

namespace kisansetu::agents {

TransporterAgent::TransporterAgent(contracts::AgentId id,
                                   contracts::NodeId baseLocation,
                                   contracts::Quintals capacity,
                                   contracts::Money costPerKm,
                                   bool isRefrigerated)
    : core::Agent(std::move(id)),
      currentLocation_(std::move(baseLocation)),
      capacity_(capacity),
      costPerKm_(costPerKm),
      isRefrigerated_(isRefrigerated),
      state_(contracts::TruckState::IDLE),
      routeStrategy_(strategy::IRoutePlanningStrategy::create(contracts::RouteStrategyType::DIJKSTRA)) {}

void TransporterAgent::setDependencies(core::EventBus* bus,
                                       contracts::EventLog* log,
                                       domain::RoadGraph* graph,
                                       std::unique_ptr<strategy::IRoutePlanningStrategy> routeStrategy) {
    eventBus_ = bus;
    eventLog_ = log;
    roadGraph_ = graph;
    if (routeStrategy != nullptr) {
        routeStrategy_ = std::move(routeStrategy);
    }
}

void TransporterAgent::onMessage(const contracts::Envelope& envelope) {
    // 1. Handle CFP
    if (envelope.performative() == contracts::Performative::CFP) {
        if (!std::holds_alternative<contracts::CfpPayload>(envelope.payload())) {
            return;
        }
        const auto& cfp = std::get<contracts::CfpPayload>(envelope.payload());

        // Check availability and capacity
        if (state_ == contracts::TruckState::IDLE && capacity_ >= cfp.volume) {
            double distance = 50.0;
            contracts::SimTime transitTime(60);

            if (roadGraph_ && routeStrategy_) {
                auto r = routeStrategy_->planRoute(*roadGraph_, cfp.origin, cfp.destination);
                if (!r.path.empty()) {
                    distance = r.distanceKm;
                    transitTime = r.estimatedTransitTime;
                    currentRoute_ = r;
                }
            }

            int64_t freightPaise = static_cast<int64_t>(distance * costPerKm_.toPaise());
            if (isRefrigerated_) {
                freightPaise = static_cast<int64_t>(freightPaise * 1.25); // Refrigeration premium
            }
            contracts::Money totalFreight(freightPaise);
            contracts::SimTime eta = cfp.readyTime + transitTime;

            contracts::ProposePayload proposal{
                .contractId = cfp.contractId,
                .bidderId = id(),
                .role = contracts::BidderRole::Transporter,
                .costOrPrice = totalFreight,
                .slotOrEta = eta,
                .ttl = contracts::SimTime(30),
                .isRefrigerated = isRefrigerated_
            };

            if (eventBus_) {
                eventBus_->send(contracts::Envelope(
                    "PROP_TRUCK_" + id().str() + "_" + std::to_string(cfp.contractId.value()),
                    cfp.contractId,
                    id(),
                    envelope.sender(), // Broker
                    contracts::Performative::PROPOSE,
                    proposal,
                    envelope.sentAt(),
                    contracts::SimTime(30)
                ));
            }
        }
    }
    // 2. Handle ACCEPT (Contract awarded)
    else if (envelope.performative() == contracts::Performative::ACCEPT) {
        if (!std::holds_alternative<contracts::AcceptPayload>(envelope.payload())) {
            return;
        }
        const auto& accept = std::get<contracts::AcceptPayload>(envelope.payload());
        if (accept.transporterId == id()) {
            state_ = contracts::TruckState::EN_ROUTE;
            activeContractId_ = accept.contractId;
            expectedArrival_ = accept.expectedArrival;

            if (eventLog_) {
                eventLog_->log(envelope.sentAt(), "TRUCK_DISPATCHED", id(), accept.mandiId,
                               activeContractId_, "En route with freight payout=" +
                               std::to_string(accept.agreedFreight.toRupees()) + " INR");
            }

            emitTelemetry(envelope.sentAt(), 1.0, "BATCH-" + std::to_string(accept.contractId.value()));
        }
    }
}

void TransporterAgent::injectStall(contracts::SimTime time) {
    state_ = contracts::TruckState::STALLED;
    if (eventLog_) {
        eventLog_->log(time, "FAULT_INJECTED_TRUCK_STALL", id(), contracts::AgentId("Broker"),
                       activeContractId_, "Truck stalled on transit corridor");
    }
    if (eventBus_) {
        contracts::FailureNoticePayload notice{
            .affectedAgentId = id(),
            .contractId = activeContractId_,
            .location = currentLocation_,
            .failureType = "TRUCK_STALL",
            .details = "Engine stall / highway stoppage",
            .timestamp = time
        };
        eventBus_->send(contracts::Envelope(
            "FAIL_NOTICE_" + id().str(),
            activeContractId_,
            id(),
            contracts::AgentId("Broker"),
            contracts::Performative::FAILURE,
            notice,
            time,
            contracts::SimTime(60)
        ));
    }
}

void TransporterAgent::injectFailure(contracts::SimTime time) {
    state_ = contracts::TruckState::FAILED;
    mechanicalHealth_ = 0.0;
    if (eventLog_) {
        eventLog_->log(time, "FAULT_INJECTED_TRUCK_FAILED", id(), contracts::AgentId("Broker"),
                       activeContractId_, "Mechanical breakdown; complete vehicle immobilisation");
    }
    if (eventBus_) {
        contracts::FailureNoticePayload notice{
            .affectedAgentId = id(),
            .contractId = activeContractId_,
            .location = currentLocation_,
            .failureType = "TRUCK_BREAKDOWN",
            .details = "Severe mechanical axle breakdown",
            .timestamp = time
        };
        eventBus_->send(contracts::Envelope(
            "FAIL_NOTICE_" + id().str(),
            activeContractId_,
            id(),
            contracts::AgentId("Broker"),
            contracts::Performative::FAILURE,
            notice,
            time,
            contracts::SimTime(60)
        ));
    }
}

void TransporterAgent::emitTelemetry(contracts::SimTime time, double currentQuality, std::string batchId) {
    contracts::TelemetryEvent te{
        .timestamp = time,
        .truckId = id(),
        .position = currentLocation_,
        .speedKmh = (state_ == contracts::TruckState::EN_ROUTE ? speedKmh_ : 0.0),
        .state = state_,
        .predictedArrival = expectedArrival_,
        .currentQuality = currentQuality,
        .batchId = std::move(batchId),
        .mandiPriceTick = std::nullopt
    };

    if (eventLog_) {
        eventLog_->log(time, "TELEMETRY_UPDATE", id(), contracts::AgentId("Watchdog"),
                       activeContractId_, te.toLogString());
    }

    if (eventBus_) {
        contracts::TelemetryUpdatePayload payload{
            .truckId = id(),
            .currentPosition = currentLocation_,
            .speedKmh = te.speedKmh,
            .state = state_,
            .predictedArrival = expectedArrival_,
            .currentQuality = currentQuality,
            .batchId = te.batchId,
            .timestamp = time
        };
        eventBus_->send(contracts::Envelope(
            "TELEM_" + id().str() + "_" + std::to_string(time.ticks()),
            activeContractId_,
            id(),
            contracts::AgentId("Watchdog"),
            contracts::Performative::TELEMETRY,
            payload,
            time,
            contracts::SimTime(10)
        ));
    }
}

} // namespace kisansetu::agents
