#pragma once

#include "contracts/Types.h"
#include <memory>
#include <string>

namespace kisansetu::domain {

class CropFreshnessState;

/**
 * @brief Base state interface for CropFreshnessState (GoF State Pattern).
 */
class CropFreshnessState {
public:
    virtual ~CropFreshnessState() = default;

    [[nodiscard]] virtual contracts::FreshnessGrade grade() const noexcept = 0;
    [[nodiscard]] virtual double priceMultiplier() const noexcept = 0;
    [[nodiscard]] virtual std::string name() const = 0;

    [[nodiscard]] virtual bool canTransitionTo(contracts::FreshnessGrade target) const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<CropFreshnessState> transitionTo(contracts::FreshnessGrade target) const = 0;

    static std::unique_ptr<CropFreshnessState> fromQuality(double quality);
    static std::unique_ptr<CropFreshnessState> create(contracts::FreshnessGrade grade);
};

} // namespace kisansetu::domain
