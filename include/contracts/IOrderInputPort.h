#pragma once

#include "Types.h"
#include <string>
#include <variant>
#include <optional>

namespace agro {

enum class IntentType {
    Sell,
    Transport,
    EmergencyStop,
    Unknown
};

struct CreateHarvestBatchCommand {
    AgentId farmerId;
    CommodityType commodity{CommodityType::Tomato};
    Quintals quantity{0.0};
    IntentType intent{IntentType::Sell};
    SimTime deadline{0};
};

struct ConfirmContractCommand {
    ContractId contractId;
    AgentId farmerId;
    bool confirmed{true};
};

struct RejectContractCommand {
    ContractId contractId;
    AgentId farmerId;
    std::string reason;
};

using DomainCommand = std::variant<
    CreateHarvestBatchCommand,
    ConfirmContractCommand,
    RejectContractCommand
>;

struct ConfirmationCard {
    std::string title;
    std::string promptHinglish;
    std::string summaryDetails;
    DomainCommand pendingCommand;
    bool requiresOneTapAction{true};
};

struct ParsedOrderResult {
    std::optional<DomainCommand> command;
    double confidence{0.0};
    std::optional<ConfirmationCard> confirmationCard;
    std::string rawInput;
    std::string interpretationNote;
};

enum class InputEventType {
    VoiceText,
    Dtmf,
    ButtonTap
};

struct InputEvent {
    InputEventType type{InputEventType::VoiceText};
    std::string payload; // vernacular string, DTMF digits, or button ID
    AgentId sourceFarmer;
    SimTime timestamp{0};
};

class IOrderInputPort {
public:
    virtual ~IOrderInputPort() = default;
    [[nodiscard]] virtual ParsedOrderResult parse(const InputEvent& event) = 0;
};

} // namespace agro
