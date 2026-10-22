#pragma once

#include "Types.h"
#include <string>
#include <variant>
#include <optional>

namespace kisansetu::contracts {

/**
 * @brief Input event from voice transcript.
 */
struct VoiceTextInput {
    std::string transcript;
    double confidence{1.0};
};

/**
 * @brief Input event from telephone keypad DTMF tones.
 */
struct DtmfInput {
    std::string keySequence;
};

/**
 * @brief Input event from physical or onscreen button press.
 */
struct ButtonTapInput {
    std::string buttonId;
};

/**
 * @brief Sum type for all user input events.
 */
using InputEvent = std::variant<VoiceTextInput, DtmfInput, ButtonTapInput>;

/**
 * @brief Typed domain command to create a harvest batch and initiate planning.
 */
struct CreateHarvestBatchCommand {
    AgentId farmerId;
    CommodityType crop{CommodityType::Tomato};
    Quintals quantity{0.0};
    Money reservePrice{0};
    SimTime readyTime{0};
    SimTime deadline{0};
    std::string rawUtterance;
    double confidenceScore{1.0};
    bool needsConfirmation{false};
};

/**
 * @brief Typed domain command to confirm an award or confirmation card.
 */
struct ConfirmContractCommand {
    ContractId contractId{0};
    AgentId farmerId;
    bool confirmed{true};
};

/**
 * @brief Typed domain command to abort an order.
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
    CancelOrderCommand
>;

/**
 * @brief Visual confirmation card displayed when voice recognition confidence is borderline.
 */
struct ConfirmationCard {
    ContractId contractId{0};
    AgentId farmerId;
    std::string summaryText;
    CommodityType crop{CommodityType::Tomato};
    Quintals quantity{0.0};
    Money expectedNetPayout{0};
    bool requiresTapToCommit{true};
};

/**
 * @brief Color classification for produce readiness display.
 */
enum class ProduceStatusColor {
    Green, // Grade A: prime condition, sell at APMC
    Amber, // Grade B/C: decaying, dispatch immediately or reroute
    Red    // Spoiled: below mandi acceptance
};

/**
 * @brief Produce status card presented to the farmer.
 */
struct ProduceStatusCard {
    std::string batchId;
    CommodityType crop;
    double currentQuality{1.0};
    FreshnessGrade grade{FreshnessGrade::GradeA};
    ProduceStatusColor color{ProduceStatusColor::Green};
    std::string actionRecommendation;
};

/**
 * @brief Port interface for parsing user inputs into typed domain commands.
 */
class IOrderInputPort {
public:
    virtual ~IOrderInputPort() = default;

    [[nodiscard]] virtual std::optional<DomainCommand> parse(
        const InputEvent& input) = 0;
};

} // namespace kisansetu::contracts
