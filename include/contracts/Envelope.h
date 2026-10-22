#pragma once

#include "Types.h"
#include "Payloads.h"
#include <variant>

namespace agro {

enum class Performative {
    CFP,
    PROPOSE,
    ACCEPT,
    REJECT,
    CANCEL,
    INFORM,
    FAILURE
};

using MessagePayload = std::variant<
    TaskAnnouncement,
    CfpPayload,
    ProposePayload,
    AcceptPayload,
    RejectPayload,
    CancelPayload,
    InformPayload,
    TelemetryUpdatePayload,
    FailureNoticePayload,
    DistressBidPayload,
    HandoffRequestPayload
>;

struct Envelope {
    uint64_t messageId{0};
    ContractId correlationId;
    AgentId sender;
    AgentId receiver;
    Performative performative{Performative::INFORM};
    MessagePayload payload;
    SimTime sentAt{0};
    SimTime ttl{0};

    [[nodiscard]] bool isExpired(SimTime currentTime) const noexcept {
        if (ttl.ticks == 0) return false;
        return currentTime.ticks > (sentAt.ticks + ttl.ticks);
    }
};

} // namespace agro
