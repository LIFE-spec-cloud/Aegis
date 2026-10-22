#pragma once

#include "Types.h"
#include <string>
#include <variant>
#include <optional>

namespace kisansetu::contracts {

enum class IntentType {
    Sell,
    Transport,
    EmergencyStop,
    Unknown
};

/**
 * @brief Typed domain command to create a harvest batch and initiate planning.
 */
struct CreateHarvestBatchCommand {
    AgentId farmerId;
    CommodityType commodity{CommodityType::Tomato};
    Quintals quantity{0.0};
    IntentType intent{IntentType::Sell};
    SimTime deadline{0};

    // Person A compatibility fields
    CommodityType crop{CommodityType::Tomato};
    Money reservePrice{0};
    SimTime readyTime{0};
    std::string rawUtterance;
    double confidenceScore{1.0};
    bool needsConfirmation{false};
};

/**
 * @brief Typed domain command to confirm an award.
 */
struct ConfirmContractCommand {
    ContractId contractId{0};
    AgentId farmerId;
    bool confirmed{true};
};

/**
 * @brief Typed domain command to reject an offer.
 */
struct RejectContractCommand {
    ContractId contractId{0};
    AgentId farmerId;
    std::string reason;
};

/**
 * @brief Typed domain command to cancel an order.
 */
struct CancelOrderCommand {
    ContractId contractId{0};
    AgentId farmerId;
    std::string reason;
};

/**
 * @brief Sum type for all domain commands parsed from user input.
 */
using DomainCommand = std::variant<
    CreateHarvestBatchCommand,
    ConfirmContractCommand,
    RejectContractCommand,
    CancelOrderCommand
>;

/**
 * @brief Visual confirmation card displayed when voice recognition confidence is borderline.
 */
struct ConfirmationCard {
    std::string title;
    std::string promptHinglish;
    std::string summaryDetails;
    DomainCommand pendingCommand{ConfirmContractCommand{}};
    bool requiresOneTapAction{true};

    // Person A compatibility fields
    ContractId contractId{0};
    AgentId farmerId;
    std::string summaryText;
    CommodityType crop{CommodityType::Tomato};
    Quintals quantity{0.0};
    Money expectedNetPayout{0};
    bool requiresTapToCommit{true};
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


/**
 * @brief Port interface for parsing user inputs into typed domain commands.
 */
class IOrderInputPort {
public:
    virtual ~IOrderInputPort() = default;

    [[nodiscard]] virtual ParsedOrderResult parse(
        const InputEvent& event) = 0;
};

} // namespace kisansetu::contracts
