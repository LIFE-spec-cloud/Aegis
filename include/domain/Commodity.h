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

    [[nodiscard]] static Commodity fromType(contracts::CommodityType type) {
        switch (type) {
            case contracts::CommodityType::Tomato:
                return Commodity{
                    .type = contracts::CommodityType::Tomato,
                    .lambda = 0.030,
                    .basePricePerQuintal = contracts::Money::fromRupees(2200.0)
                };
            case contracts::CommodityType::Onion:
                return Commodity{
                    .type = contracts::CommodityType::Onion,
                    .lambda = 0.008,
                    .basePricePerQuintal = contracts::Money::fromRupees(1800.0)
                };
            case contracts::CommodityType::Chilli:
                return Commodity{
                    .type = contracts::CommodityType::Chilli,
                    .lambda = 0.020,
                    .basePricePerQuintal = contracts::Money::fromRupees(6000.0)
                };
        }
        return Commodity{};
    }
};

} // namespace kisansetu::domain
