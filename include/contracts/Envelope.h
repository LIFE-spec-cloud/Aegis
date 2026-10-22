#pragma once

#include "Types.h"
#include "Payloads.h"
#include <string>

namespace kisansetu::contracts {

/**
 * @brief Immutable message envelope routed by the EventBus and LogisticsBroker.
 *
 * All fields are immutable after construction.
 */
class Envelope {
public:
    Envelope(std::string messageId,
             ContractId correlationId,
             AgentId sender,
             AgentId receiver,
             Performative performative,
             Payload payload,
             SimTime sentAt,
             SimTime ttl)
        : messageId_(std::move(messageId)),
          correlationId_(correlationId),
          sender_(std::move(sender)),
          receiver_(std::move(receiver)),
          performative_(performative),
          payload_(std::move(payload)),
          sentAt_(sentAt),
          ttl_(ttl) {}

    [[nodiscard]] const std::string& messageId() const noexcept { return messageId_; }
    [[nodiscard]] ContractId correlationId() const noexcept { return correlationId_; }
    [[nodiscard]] const AgentId& sender() const noexcept { return sender_; }
    [[nodiscard]] const AgentId& receiver() const noexcept { return receiver_; }
    [[nodiscard]] Performative performative() const noexcept { return performative_; }
    [[nodiscard]] const Payload& payload() const noexcept { return payload_; }
    [[nodiscard]] SimTime sentAt() const noexcept { return sentAt_; }
    [[nodiscard]] SimTime ttl() const noexcept { return ttl_; }
    [[nodiscard]] bool isExpired(SimTime currentTime) const noexcept {
        return currentTime > (sentAt_ + ttl_);
    }

private:
    std::string messageId_;
    ContractId correlationId_;
    AgentId sender_;
    AgentId receiver_;
    Performative performative_;
    Payload payload_;
    SimTime sentAt_;
    SimTime ttl_;
};

} // namespace kisansetu::contracts
