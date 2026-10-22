#pragma once

#include <random>
#include <cstdint>

namespace kisansetu::core {

/**
 * @brief Deterministic pseudo-random number generator owned by the simulation.
 */
class Rng {
public:
    explicit Rng(uint64_t seed = 42) : engine_(seed), initialSeed_(seed) {}

    void seed(uint64_t s) {
        initialSeed_ = s;
        engine_.seed(s);
    }

    [[nodiscard]] uint64_t initialSeed() const noexcept { return initialSeed_; }

    [[nodiscard]] uint64_t nextU64() { return engine_(); }

    [[nodiscard]] double nextDouble(double min = 0.0, double max = 1.0) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(engine_);
    }

    [[nodiscard]] int64_t nextInt(int64_t min, int64_t max) {
        std::uniform_int_distribution<int64_t> dist(min, max);
        return dist(engine_);
    }

    [[nodiscard]] std::mt19937_64& engine() noexcept { return engine_; }

private:
    std::mt19937_64 engine_;
    uint64_t initialSeed_{42};
};

} // namespace kisansetu::core
