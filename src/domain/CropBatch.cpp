#include "domain/CropBatch.h"
#include <cmath>
#include <algorithm>

namespace kisansetu::domain {

CropBatch::CropBatch(std::string batchId,
                     Commodity commodity,
                     contracts::Quintals volume,
                     contracts::SimTime harvestTime)
    : batchId_(std::move(batchId)),
      commodity_(commodity),
      volume_(volume),
      harvestTime_(harvestTime),
      currentQuality_(1.0),
      state_(CropFreshnessState::fromQuality(1.0)) {}

double CropBatch::calculateMuTemp(double ambientTempC, bool isRefrigerated) {
    double mu = 1.0;
    if (ambientTempC > 20.0) {
        double clampedTemp = std::min(ambientTempC, 40.0);
        mu = 1.0 + 0.5 * ((clampedTemp - 20.0) / 20.0);
    }
    if (isRefrigerated) {
        mu *= 0.6;
    }
    return mu;
}

double CropBatch::predictQualityAt(contracts::SimTime futureTime, double ambientTempC, bool isRefrigerated) const {
    if (futureTime <= harvestTime_) {
        return 1.0;
    }
    double elapsedHours = contracts::Hours(futureTime - harvestTime_).value();
    double mu = calculateMuTemp(ambientTempC, isRefrigerated);
    double q = 1.0 * std::exp(-commodity_.lambda * elapsedHours * mu);
    return std::max(0.0, std::min(1.0, q));
}

void CropBatch::updateQuality(contracts::SimTime currentTime, double ambientTempC, bool isRefrigerated) {
    currentQuality_ = predictQualityAt(currentTime, ambientTempC, isRefrigerated);
    state_ = CropFreshnessState::fromQuality(currentQuality_);
}

} // namespace kisansetu::domain
