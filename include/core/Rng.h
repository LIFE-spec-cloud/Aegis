#pragma once

#include <random>
#include <cstdint>

namespace kisansetu::core {

/**
 * @brief Deterministic pseudo-random number generator owned by the simulation.
 *
 * Uses std::mt19937_64. No std::random_device is used in the simulation engine.
 */
class Rng {
public:
    explicit Rng(uint64_t seed = 42) : engine_(seed), initialSeed_(seed) {}

    void seed(uint64_t s) noexcept {
        initialSeed_ = s;
        engine_.seed(s);
    }

    [[nodiscard]] uint64_t initialSeed() const noexcept { return initialSeed_; }

    [[nodiscard]] uint64_t nextU64() noexcept { return engine_(); }

    [[nodiscard]] double uniformDouble(double min = 0.0, double max = 1.0) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(engine_);
    }

    [[nodiscard]] int64_t uniformInt(int64_t min, int64_t max) {
        std::uniform_int_distribution<int64_t> dist(min, max);
        return dist(engine_);
    }

    [[nodiscard]] bool bernoulli(double probability) {
        std::bernoulli_distribution dist(probability);
        return dist(engine_);
    }

    [[nodiscard]] double gaussian(double mean, double stddev) {
        std::normal_distribution<double> dist(mean, stddev);
        return dist(engine_);
    }

    [[nodiscard]] std::mt19937_64& engine() noexcept { return engine_; }

private:
    std::mt19937_64 engine_;
    uint64_t initialSeed_{42};
};

} // namespace kisansetu::core
