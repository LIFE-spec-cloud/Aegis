#pragma once

#include "contracts/Types.h"
#include "contracts/IRecoveryContext.h"
#include "domain/RoadGraph.h"
#include <memory>

namespace kisansetu::strategy {

/**
 * @brief Strategy pattern interface for route planning across the RoadGraph.
 */
class IRoutePlanningStrategy {
public:
    virtual ~IRoutePlanningStrategy() = default;

    [[nodiscard]] virtual contracts::RouteInfo planRoute(
        const domain::RoadGraph& graph,
        const contracts::NodeId& origin,
        const contracts::NodeId& destination) const = 0;

    [[nodiscard]] virtual contracts::RouteStrategyType type() const noexcept = 0;

    static std::unique_ptr<IRoutePlanningStrategy> create(contracts::RouteStrategyType type);
};

} // namespace kisansetu::strategy
