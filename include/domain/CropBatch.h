#pragma once

#include "contracts/Types.h"
#include "domain/Commodity.h"
#include "domain/CropFreshnessState.h"
#include <string>
#include <memory>

namespace kisansetu::domain {

/**
 * @brief Represents a physical harvest batch subject to biological decay.
 */
class CropBatch {
public:
    CropBatch(std::string batchId,
              Commodity commodity,
              contracts::Quintals volume,
              contracts::SimTime harvestTime);

    [[nodiscard]] const std::string& batchId() const noexcept { return batchId_; }
    [[nodiscard]] const Commodity& commodity() const noexcept { return commodity_; }
    [[nodiscard]] contracts::Quintals volume() const noexcept { return volume_; }
    [[nodiscard]] contracts::SimTime harvestTime() const noexcept { return harvestTime_; }

    [[nodiscard]] double currentQuality() const noexcept { return currentQuality_; }
    [[nodiscard]] const CropFreshnessState& state() const noexcept { return *state_; }

    /**
     * @brief Calculate temperature acceleration factor mu_temp.
     * 1.0 for <= 20C, rising linearly to 1.5 at 40C.
     * If refrigerated, multiplied by 0.6.
     */
    [[nodiscard]] static double calculateMuTemp(double ambientTempC, bool isRefrigerated);

    /**
     * @brief Compute predicted quality at future time t given temperature and refrigeration.
     */
    [[nodiscard]] double predictQualityAt(contracts::SimTime futureTime, double ambientTempC, bool isRefrigerated) const;

    /**
     * @brief Advance time and apply biological exponential decay.
     */
    void updateQuality(contracts::SimTime currentTime, double ambientTempC, bool isRefrigerated);

private:
    std::string batchId_;
    Commodity commodity_;
    contracts::Quintals volume_;
    contracts::SimTime harvestTime_;
    double currentQuality_{1.0};
    std::unique_ptr<CropFreshnessState> state_;
};

} // namespace kisansetu::domain
