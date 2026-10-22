#include "agents/APMCMandiAgent.h"
#include "contracts/Payloads.h"
#include "domain/Commodity.h"

namespace kisansetu::agents {

APMCMandiAgent::APMCMandiAgent(contracts::AgentId id,
                               contracts::NodeId location,
                               contracts::Money basePrice,
                               size_t gateQuotaPerHour)
    : core::Agent(std::move(id)),
      location_(std::move(location)),
      basePrice_(basePrice),
      gateQuotaPerHour_(gateQuotaPerHour),
      pricingStrategy_(strategy::createSurgePricingStrategy(0.50, 0.40)) {}

void APMCMandiAgent::setDependencies(core::EventBus* bus,
                                     contracts::EventLog* log,
                                     std::unique_ptr<strategy::IPricingStrategy> pricingStrategy) {
    eventBus_ = bus;
    eventLog_ = log;
    if (pricingStrategy != nullptr) {
        pricingStrategy_ = std::move(pricingStrategy);
    }
}

size_t APMCMandiAgent::getOccupancy(contracts::SimTime slotTime) const {
    int64_t hour = slotTime.ticks() / 60;
    auto it = hourlySlots_.find(hour);
    if (it != hourlySlots_.end()) {
        return it->second.size();
    }
    return 0;
}

contracts::Money APMCMandiAgent::quoteCurrentPrice(contracts::CommodityType crop, contracts::SimTime slotTime) const {
    size_t occ = getOccupancy(slotTime);
    domain::Commodity comm = domain::Commodity::fromType(crop);
    comm.basePricePerQuintal = basePrice_;
    if (pricingStrategy_) {
        return pricingStrategy_->quotePrice(comm, slotTime, occ);
    }
    return basePrice_;
}

std::optional<contracts::SlotReservationHandle> APMCMandiAgent::reserveSlot(
    const contracts::AgentId& mandiId,
    const contracts::ContractId& contractId,
    contracts::SimTime slotTime) {
    if (mandiId != id()) {
        return std::nullopt;
    }

    int64_t hour = slotTime.ticks() / 60;
    auto& slots = hourlySlots_[hour];
    if (slots.size() >= gateQuotaPerHour_) {
        if (eventLog_) {
            eventLog_->log(slotTime, "GATE_SLOT_REJECTED_QUOTA_FULL", id(), contracts::AgentId("-"),
                           contractId, "Hour=" + std::to_string(hour) + " Quota=" + std::to_string(gateQuotaPerHour_));
        }
        return std::nullopt; // Strictly serialized quota
    }

    slots.push_back(contractId);
    uint64_t resId = ++nextSlotReservationId_;

    if (eventLog_) {
        eventLog_->log(slotTime, "GATE_SLOT_RESERVED", id(), contracts::AgentId("-"),
                       contractId, "ReservationId=" + std::to_string(resId) +
                       " Hour=" + std::to_string(hour) + " Occupancy=" + std::to_string(slots.size()));
    }

    return contracts::SlotReservationHandle(id(), contractId, slotTime, resId);
}

bool APMCMandiAgent::cancelSlot(contracts::SlotReservationHandle& handle) {
    if (!handle.isValid() || handle.mandiId() != id()) {
        return false;
    }

    int64_t hour = handle.slotTime().ticks() / 60;
    auto it = hourlySlots_.find(hour);
    if (it != hourlySlots_.end()) {
        auto& vec = it->second;
        for (auto vIt = vec.begin(); vIt != vec.end(); ++vIt) {
            if (*vIt == handle.contractId()) {
                vec.erase(vIt);
                break;
            }
        }
    }

    if (eventLog_) {
        eventLog_->log(handle.slotTime(), "GATE_SLOT_CANCELLED", id(), contracts::AgentId("-"),
                       handle.contractId(), "Slot released, ReservationId=" + std::to_string(handle.reservationId()));
    }

    handle.invalidate();
    return true;
}

void APMCMandiAgent::onMessage(const contracts::Envelope& envelope) {
    if (envelope.performative() == contracts::Performative::CFP) {
        if (!std::holds_alternative<contracts::CfpPayload>(envelope.payload())) {
            return;
        }
        const auto& cfp = std::get<contracts::CfpPayload>(envelope.payload());

        // Check gate slot availability in the desired window
        contracts::SimTime candidateSlot = cfp.readyTime + contracts::SimTime(60); // standard transit window
        if (getOccupancy(candidateSlot) < gateQuotaPerHour_) {
            contracts::Money quotedPrice = quoteCurrentPrice(cfp.crop, candidateSlot);

            contracts::ProposePayload proposal{
                .contractId = cfp.contractId,
                .bidderId = id(),
                .role = contracts::BidderRole::Mandi,
                .costOrPrice = quotedPrice,
                .slotOrEta = candidateSlot,
                .ttl = contracts::SimTime(30),
                .isRefrigerated = false
            };

            if (eventBus_) {
                eventBus_->send(contracts::Envelope(
                    "PROP_MANDI_" + id().str() + "_" + std::to_string(cfp.contractId.value()),
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
}

contracts::Money APMCMandiAgent::clearCallAuction(contracts::SimTime windowTime, contracts::CommodityType /*crop*/) {
    size_t arrivals = getOccupancy(windowTime);
    contracts::Money clearingPrice = basePrice_;
    if (pricingStrategy_) {
        clearingPrice = pricingStrategy_->clearAuction(basePrice_, arrivals, static_cast<double>(gateQuotaPerHour_));
    }

    if (eventLog_) {
        eventLog_->log(windowTime, "AUCTION_CLEARED", id(), contracts::AgentId("Broadcast"),
                       contracts::ContractId(0),
                       "Arrivals=" + std::to_string(arrivals) +
                       " BasePrice=" + std::to_string(basePrice_.toRupees()) +
                       " ClearingPrice=" + std::to_string(clearingPrice.toRupees()));
    }

    return clearingPrice;
}

} // namespace kisansetu::agents
