#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <optional>

namespace agro {

struct TaskAnnouncement {
    AgentId farmerId;
    CropBatch batch;
    SimTime readyTime;
    SimTime deadline;
    Money reservePrice;
    double riskAversion{0.1};
};

struct CfpPayload {
    ContractId contractId;
    AgentId farmerId;
    CropBatch batch;
    std::string originNode;
    SimTime readyTime;
    SimTime deadline;
};

struct TransporterProposal {
    AgentId transporterId;
    Money freightCost;
    SimTime estimatedTransitTime;
    bool refrigeratedOption{false};
    SimTime expiresAt;
};

struct MandiProposal {
    AgentId mandiId;
    SimTime slotTime;
    Money quotedPricePerQuintal;
    Money mandiFee;
    SimTime expiresAt;
};

struct ProposePayload {
    ContractId contractId;
    std::optional<TransporterProposal> transporterOffer;
    std::optional<MandiProposal> mandiOffer;
};

struct AcceptPayload {
    ContractId contractId;
    AgentId awardedTransporter;
    AgentId awardedMandi;
    Money agreedFreight;
    Money expectedPricePerQuintal;
    SimTime reservedSlotTime;
};

struct RejectPayload {
    ContractId contractId;
    std::string reason;
};

struct CancelPayload {
    ContractId contractId;
    std::string reason;
};

struct InformPayload {
    ContractId contractId;
    std::string statusMessage;
};

struct TelemetryUpdatePayload {
    AgentId truckId;
    ContractId contractId;
    std::string currentNode;
    double speedKmH{0.0};
    TruckState state{TruckState::IDLE};
    SimTime currentSimTime{0};
    SimTime eta{0};
    double currentQuality{1.0};
    double predictedArrivalQuality{1.0};
};

struct FailureNoticePayload {
    ContractId contractId;
    AgentId reportingAgent;
    std::string failureType; // "HIGHWAY_STOPPAGE", "MANDI_GLUT", "ENGINE_FAILURE"
    SimTime occurredAt;
    std::string details;
};

struct DistressBidPayload {
    AgentId processorId;
    ContractId contractId;
    Money bidPricePerQuintal;
    double maxDistanceKm{30.0};
    SimTime expiresAt;
};

struct HandoffRequestPayload {
    ContractId contractId;
    AgentId failedTruckId;
    std::string rendezvousNode;
    CropBatch batch;
    SimTime rendezvousTime;
};

} // namespace agro
