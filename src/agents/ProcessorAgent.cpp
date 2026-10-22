#include "agents/ProcessorAgent.h"
#include "contracts/Payloads.h"

namespace kisansetu::agents {

ProcessorAgent::ProcessorAgent(contracts::AgentId id,
                               contracts::NodeId location,
                               contracts::Quintals intakeCapacity,
                               contracts::Money priceFloor)
    : core::Agent(std::move(id)),
      location_(std::move(location)),
      intakeCapacity_(intakeCapacity),
      priceFloor_(priceFloor) {}

void ProcessorAgent::setDependencies(core::EventBus* bus, contracts::EventLog* log) {
    eventBus_ = bus;
    eventLog_ = log;
}

void ProcessorAgent::onMessage(const contracts::Envelope& envelope) {
    if (envelope.performative() == contracts::Performative::DISTRESS_NOTICE) {
        if (!std::holds_alternative<contracts::DistressAuctionNoticePayload>(envelope.payload())) {
            return;
        }
        const auto& notice = std::get<contracts::DistressAuctionNoticePayload>(envelope.payload());

        // Processor accepts down to 0.40 quality
        if (notice.currentQuality >= 0.40 && intakeCapacity_ >= notice.volume) {
            // Price bid is discounted grade multiplier (Grade C multiplier ~0.45 of base)
            contracts::Money bidPrice = priceFloor_;
            if (notice.currentQuality >= 0.50) {
                bidPrice = priceFloor_ * 1.15;
            }

            contracts::DistressBidPayload bid{
                .contractId = notice.contractId,
                .processorId = id(),
                .bidPricePerQuintal = bidPrice,
                .maxVolumeAccepted = notice.volume,
                .processorLocation = location_,
                .expirationTime = notice.bidWindowCloses
            };

            if (eventLog_) {
                eventLog_->log(envelope.sentAt(), "DISTRESS_BID_SUBMITTED", id(), envelope.sender(),
                               notice.contractId, "Bid=" + std::to_string(bidPrice.toRupees()) +
                               " INR/qtl for Q=" + std::to_string(notice.currentQuality));
            }

            if (eventBus_) {
                eventBus_->send(contracts::Envelope(
                    "DISTRESS_BID_" + id().str() + "_" + std::to_string(notice.contractId.value()),
                    notice.contractId,
                    id(),
                    envelope.sender(), // Broker
                    contracts::Performative::DISTRESS_BID,
                    bid,
                    envelope.sentAt(),
                    contracts::SimTime(30)
                ));
            }
        }
    }
}

} // namespace kisansetu::agents
