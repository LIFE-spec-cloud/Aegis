#include "domain/CropFreshnessState.h"
#include <stdexcept>

namespace kisansetu::domain {

namespace {

class GradeAState final : public CropFreshnessState {
public:
    [[nodiscard]] contracts::FreshnessGrade grade() const noexcept override {
        return contracts::FreshnessGrade::GradeA;
    }
    [[nodiscard]] double priceMultiplier() const noexcept override { return 1.00; }
    [[nodiscard]] std::string name() const override { return "GradeA"; }

    [[nodiscard]] bool canTransitionTo(contracts::FreshnessGrade /*target*/) const noexcept override {
        // Can stay GradeA or decay to GradeB, GradeC, or Spoiled
        return true;
    }

    [[nodiscard]] std::unique_ptr<CropFreshnessState> transitionTo(contracts::FreshnessGrade target) const override {
        if (!canTransitionTo(target)) {
            throw std::logic_error("Illegal transition from GradeA");
        }
        return CropFreshnessState::create(target);
    }
};

class GradeBState final : public CropFreshnessState {
public:
    [[nodiscard]] contracts::FreshnessGrade grade() const noexcept override {
        return contracts::FreshnessGrade::GradeB;
    }
    [[nodiscard]] double priceMultiplier() const noexcept override { return 0.75; }
    [[nodiscard]] std::string name() const override { return "GradeB"; }

    [[nodiscard]] bool canTransitionTo(contracts::FreshnessGrade target) const noexcept override {
        // Cannot recover back to GradeA
        return target != contracts::FreshnessGrade::GradeA;
    }

    [[nodiscard]] std::unique_ptr<CropFreshnessState> transitionTo(contracts::FreshnessGrade target) const override {
        if (!canTransitionTo(target)) {
            throw std::logic_error("Illegal transition from GradeB to GradeA");
        }
        return CropFreshnessState::create(target);
    }
};

class GradeCState final : public CropFreshnessState {
public:
    [[nodiscard]] contracts::FreshnessGrade grade() const noexcept override {
        return contracts::FreshnessGrade::GradeC;
    }
    [[nodiscard]] double priceMultiplier() const noexcept override { return 0.45; }
    [[nodiscard]] std::string name() const override { return "GradeC"; }

    [[nodiscard]] bool canTransitionTo(contracts::FreshnessGrade target) const noexcept override {
        // Can only transition to GradeC or Spoiled
        return target == contracts::FreshnessGrade::GradeC || target == contracts::FreshnessGrade::Spoiled;
    }

    [[nodiscard]] std::unique_ptr<CropFreshnessState> transitionTo(contracts::FreshnessGrade target) const override {
        if (!canTransitionTo(target)) {
            throw std::logic_error("Illegal transition from GradeC to higher grade");
        }
        return CropFreshnessState::create(target);
    }
};

class SpoiledState final : public CropFreshnessState {
public:
    [[nodiscard]] contracts::FreshnessGrade grade() const noexcept override {
        return contracts::FreshnessGrade::Spoiled;
    }
    [[nodiscard]] double priceMultiplier() const noexcept override { return 0.00; }
    [[nodiscard]] std::string name() const override { return "Spoiled"; }

    [[nodiscard]] bool canTransitionTo(contracts::FreshnessGrade target) const noexcept override {
        // Spoiled is a terminal state; can never recover
        return target == contracts::FreshnessGrade::Spoiled;
    }

    [[nodiscard]] std::unique_ptr<CropFreshnessState> transitionTo(contracts::FreshnessGrade target) const override {
        if (!canTransitionTo(target)) {
            throw std::logic_error("Illegal transition: Spoiled produce cannot recover");
        }
        return CropFreshnessState::create(target);
    }
};

} // namespace

std::unique_ptr<CropFreshnessState> CropFreshnessState::create(contracts::FreshnessGrade grade) {
    switch (grade) {
        case contracts::FreshnessGrade::GradeA:  return std::make_unique<GradeAState>();
        case contracts::FreshnessGrade::GradeB:  return std::make_unique<GradeBState>();
        case contracts::FreshnessGrade::GradeC:  return std::make_unique<GradeCState>();
        case contracts::FreshnessGrade::Spoiled: return std::make_unique<SpoiledState>();
    }
    return std::make_unique<SpoiledState>();
}

std::unique_ptr<CropFreshnessState> CropFreshnessState::fromQuality(double quality) {
    if (quality >= 0.85) {
        return std::make_unique<GradeAState>();
    }
    if (quality >= 0.65) {
        return std::make_unique<GradeBState>();
    }
    if (quality >= 0.40) {
        return std::make_unique<GradeCState>();
    }
    return std::make_unique<SpoiledState>();
}

} // namespace kisansetu::domain
