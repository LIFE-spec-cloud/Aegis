#pragma once

#include "../commands/TransitContractCommand.h"
#include "../include/contracts/IRecoveryContext.h"
#include "../include/contracts/ITelemetry.h"
#include <string>

namespace agro {

struct RecoveryResult {
    bool success{false};
    std::string strategyName;
    std::string actionSummary;
    AgentId newDestinationOrPeer;
    Money finalPriceOrCost{0};
    uint64_t contractVersionAfterRecovery{0};
};

class IFailureRecovery {
public:
    virtual ~IFailureRecovery() = default;

    [[nodiscard]] virtual RecoveryResult recover(
        TransitContractCommand& contract,
        const IRecoveryContext& context,
        const AnomalyAlert& alert,
        SimTime currentTime) = 0;
};

} // namespace agro
