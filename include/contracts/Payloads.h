#pragma once

#include "Types.h"
#include <string>
#include <variant>
#include <optional>

namespace kisansetu::contracts {

/**
 * @brief Farmer announces a harvest batch to the LogisticsBroker.
 */
struct TaskAnnouncementPayload {
    AgentId farmerId;
    CommodityType crop;
    Quintals volume;
    SimTime readyTime;
    SimTime deadline;
    Money reservePricePerQuintal;
    NodeId farmLocation;
};

using TaskAnnouncement = TaskAnnouncementPayload;

/**
 * @brief Broker fans out Call for Proposals (CFP) to Transporters and Mandis.
 */
struct CfpPayload {
    ContractId contractId;
    AgentId farmerId;
    CommodityType crop;
    Quintals volume;
    NodeId origin;
    NodeId destination; // Preferred or candidate mandi
    SimTime readyTime;
    SimTime deadline;
};

/**
 * @brief Role of the proposing agent.
 */
enum class BidderRole {
    Transporter,
    Mandi
};

/**
 * @brief Proposal submitted by a Transporter or Mandi.
 */
struct ProposePayload {
    ContractId contractId;
    AgentId bidderId;
    BidderRole role;
    Money costOrPrice;       // Freight cost for Transporter; forecast price quote for Mandi
    SimTime slotOrEta;       // True ETA for Transporter; scheduled gate slot for Mandi
    SimTime ttl;             // Expiration time of proposal
    bool isRefrigerated{false};
};

/**
 * @brief Farmer awards the bundled contract to selected Transporter and Mandi.
 */
struct AcceptPayload {
    ContractId contractId;
    AgentId farmerId;
    AgentId transporterId;
    AgentId mandiId;
    Money agreedFreight;
    Money expectedMandiPrice;
    SimTime scheduledGateSlot;
    SimTime expectedArrival;
};

/**
 * @brief Proposal or task rejection notice.
 */
struct RejectPayload {
    ContractId contractId;
    AgentId rejectingAgentId;
    std::string reason;
};

/**
 * @brief Cancellation notice for an in-flight or negotiated contract.
 */
struct CancelPayload {
    ContractId contractId;
    AgentId cancellingAgentId;
    std::string reason;
};

/**
 * @brief Telemetry update broadcast by Transporters while in transit.
 */
struct TelemetryUpdatePayload {
    AgentId truckId;
    NodeId currentPosition;
    double speedKmh{0.0};
    TruckState state{TruckState::IDLE};
    SimTime predictedArrival{0};
    double currentQuality{1.0};
    std::string batchId;
    SimTime timestamp{0};
};

/**
 * @brief Anomaly or physical failure notification.
 */
struct FailureNoticePayload {
    AgentId affectedAgentId;
    ContractId contractId;
    NodeId location;
    std::string failureType; // "SPOILAGE_BREACH", "MANDI_GLUT", "TRUCK_BREAKDOWN"
    std::string details;
    SimTime timestamp{0};
};

/**
 * @brief Broker initiates distress auction to processing plants.
 */
struct DistressAuctionNoticePayload {
    ContractId contractId;
    CommodityType crop;
    Quintals volume;
    NodeId currentLocation;
    double currentQuality{1.0};
    SimTime bidWindowCloses{0};
};

/**
 * @brief Processing plant bids on distressed cargo.
 */
struct DistressBidPayload {
    ContractId contractId;
    AgentId processorId;
    Money bidPricePerQuintal;
    Quintals maxVolumeAccepted;
    NodeId processorLocation;
    SimTime expirationTime{0};
};

/**
 * @brief Order to divert truck to an alternate destination.
 */
struct RerouteOrderPayload {
    ContractId contractId;
    AgentId truckId;
    NodeId originalDestination;
    NodeId newDestination;
    SimTime newSlotTime{0};
    Money revisedTollAndFreight;
};

/**
 * @brief Request peer handoff from a disabled truck to a backup truck.
 */
struct HandoffRequestPayload {
    ContractId contractId;
    AgentId disabledTruckId;
    AgentId backupTruckId;
    NodeId rendezvousLocation;
    SimTime rendezvousEta{0};
    Quintals cargoVolume;
};

/**
 * @brief Backup truck response to handoff request.
 */
struct HandoffAcceptPayload {
    ContractId contractId;
    AgentId backupTruckId;
    bool accepted{false};
    SimTime backupTruckEta{0};
};

/**
 * @brief Mandi market price tick update.
 */
struct MandiPriceTickPayload {
    AgentId mandiId;
    CommodityType crop;
    Money currentPricePerQuintal;
    size_t arrivalsCount{0};
    bool glutFlag{false};
    SimTime timestamp{0};
};

/**
 * @brief Generic informational payload.
 */
struct InformPayload {
    std::string message;
};

/**
 * @brief Sum type of all message payloads in KisanSetu.
 */
using Payload = std::variant<
    TaskAnnouncementPayload,
    CfpPayload,
    ProposePayload,
    AcceptPayload,
    RejectPayload,
    CancelPayload,
    TelemetryUpdatePayload,
    FailureNoticePayload,
    DistressAuctionNoticePayload,
    DistressBidPayload,
    RerouteOrderPayload,
    HandoffRequestPayload,
    HandoffAcceptPayload,
    MandiPriceTickPayload,
    InformPayload
>;

using MessagePayload = Payload;

} // namespace kisansetu::contracts
