#include "domain/PriceModel.h"
#include <algorithm>

namespace kisansetu::domain {

contracts::Money PriceModel::calculateSurgePrice(
    contracts::Money basePrice,
    double arrivals,
    double absorptionCapacity,
    double sensitivityK,
    double floorMultiplier) {
    if (absorptionCapacity <= 0.0) {
        return basePrice * floorMultiplier;
    }

    double ratio = arrivals / absorptionCapacity;
    double overload = std::max(0.0, ratio - 1.0);
    double discountFactor = sensitivityK * overload;
    double multiplier = std::max(floorMultiplier, 1.0 - discountFactor);

    return basePrice * multiplier;
}

contracts::Money PriceModel::applyGradeMultiplier(
    contracts::Money price,
    double gradeMultiplier) {
    return price * gradeMultiplier;
}

contracts::Money PriceModel::calculateNetMargin(
    contracts::Money mandiPricePerQuintal,
    double gradeMultiplier,
    contracts::Quintals volume,
    contracts::Money transitCost,
    contracts::Money mandiFees) {
    // Gross payout = (price * gradeMultiplier) * volume
    contracts::Money effectivePrice = mandiPricePerQuintal * gradeMultiplier;
    int64_t grossPaise = static_cast<int64_t>(effectivePrice.toPaise() * volume.value());
    contracts::Money gross(grossPaise);

    return gross - transitCost - mandiFees;
}

} // namespace kisansetu::domain
