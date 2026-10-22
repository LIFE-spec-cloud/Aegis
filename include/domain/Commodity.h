#pragma once

#include "contracts/Types.h"
#include <string>

namespace kisansetu::domain {

/**
 * @brief Domain properties of an agricultural commodity.
 */
struct Commodity {
    contracts::CommodityType type{contracts::CommodityType::Tomato};
    double lambda{0.030};                 // Exponential decay rate per hour
    contracts::Money basePricePerQuintal{contracts::Money::fromRupees(2200.0)};

    [[nodiscard]] std::string name() const {
        return contracts::toString(type);
    }
};

} // namespace kisansetu::domain
