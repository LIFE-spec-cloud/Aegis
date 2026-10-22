#pragma once

#include "contracts/Types.h"

namespace kisansetu::core {

/**
 * @brief Simulation clock representing discrete event time.
 */
class SimClock {
public:
    constexpr SimClock() noexcept : current_(0) {}
    constexpr explicit SimClock(contracts::SimTime start) noexcept : current_(start) {}

    [[nodiscard]] constexpr contracts::SimTime now() const noexcept { return current_; }
    constexpr void advanceTo(contracts::SimTime t) noexcept { current_ = t; }
    constexpr void advanceBy(contracts::SimTime dt) noexcept { current_ += dt; }
    constexpr void reset() noexcept { current_ = contracts::SimTime(0); }

private:
    contracts::SimTime current_{0};
};

} // namespace kisansetu::core
