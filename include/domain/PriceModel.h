#pragma once

#include "contracts/Types.h"
#include <algorithm>

namespace kisansetu::domain {

/**
 * @brief Economic pricing model implementing APMC surge pricing and net margin formulas.
 */
class PriceModel {
public:
    /**
     * @brief Calculate clearing price under surge dynamics.
     * P = P_base * (1 - k * max(0, arrivals / absorption_capacity - 1)), clamped to >= 0.4 * P_base.
     */
    [[nodiscard]] static contracts::Money calculateSurgePrice(
        contracts::Money basePrice,
        double arrivals,
        double absorptionCapacity,
        double sensitivityK = 0.50,
        double floorMultiplier = 0.40);

    /**
     * @brief Compute effective price for a given freshness grade multiplier.
     */
    [[nodiscard]] static contracts::Money applyGradeMultiplier(
        contracts::Money price,
        double gradeMultiplier);

    /**
     * @brief Calculate net margin for a given mandi destination.
     * NetMargin = (P_m * gradeMultiplier * Volume) - TransitCost - MandiFees
     */
    [[nodiscard]] static contracts::Money calculateNetMargin(
        contracts::Money mandiPricePerQuintal,
        double gradeMultiplier,
        contracts::Quintals volume,
        contracts::Money transitCost,
        contracts::Money mandiFees);
};

} // namespace kisansetu::domain
