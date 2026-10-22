#pragma once

#include "contracts/Types.h"
#include "domain/Commodity.h"
#include <memory>

namespace kisansetu::strategy {

/**
 * @brief Strategy pattern interface for mandi and broker price quoting and auction clearing.
 */
class IPricingStrategy {
public:
    virtual ~IPricingStrategy() = default;

    [[nodiscard]] virtual contracts::Money quotePrice(
        const domain::Commodity& commodity,
        contracts::SimTime slotTime,
        size_t currentQueueLength) const = 0;

    [[nodiscard]] virtual contracts::Money clearAuction(
        contracts::Money basePrice,
        size_t totalArrivals,
        double absorptionCapacity) const = 0;
};

} // namespace kisansetu::strategy
