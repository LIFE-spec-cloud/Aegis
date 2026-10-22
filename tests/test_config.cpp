#include "catch_compat.hpp"
#include "core/ConfigLoader.h"
#include <cmath>

using namespace kisansetu::core;

TEST_CASE("ConfigLoader_DefaultParameters", "[config]") {
    ConfigLoader config;
    bool loaded = config.load("config/default.cfg");
    REQUIRE(loaded);

    // Simulation section
    CHECK(config.getInt("simulation.seed") == 42);
    CHECK(config.getInt("simulation.ticks_per_hour") == 60);
    CHECK(std::abs(config.getDouble("simulation.ambient_temperature_celsius") - 25.0) < 1e-4);
    CHECK(std::abs(config.getDouble("simulation.refrigerated_decay_factor") - 0.6) < 1e-4);

    // Commodities
    CHECK(std::abs(config.getDouble("commodities.tomato.lambda") - 0.030) < 1e-4);
    CHECK(config.getMoney("commodities.tomato.base_price").toRupees() == 2200.0);

    CHECK(std::abs(config.getDouble("commodities.onion.lambda") - 0.008) < 1e-4);
    CHECK(config.getMoney("commodities.onion.base_price").toRupees() == 1800.0);

    CHECK(std::abs(config.getDouble("commodities.chilli.lambda") - 0.020) < 1e-4);
    CHECK(config.getMoney("commodities.chilli.base_price").toRupees() == 6000.0);

    // Grades
    CHECK(std::abs(config.getDouble("grades.threshold_a") - 0.85) < 1e-4);
    CHECK(std::abs(config.getDouble("grades.threshold_b") - 0.65) < 1e-4);
    CHECK(std::abs(config.getDouble("grades.threshold_c") - 0.40) < 1e-4);
    CHECK(std::abs(config.getDouble("grades.mandi_min_quality") - 0.60) < 1e-4);
    CHECK(std::abs(config.getDouble("grades.processor_min_quality") - 0.40) < 1e-4);

    // Surge model
    CHECK(std::abs(config.getDouble("surge_model.sensitivity_k") - 0.50) < 1e-4);
    CHECK(std::abs(config.getDouble("surge_model.absorption_capacity") - 4.0) < 1e-4);

    // APMC
    CHECK(config.getInt("apmc.gate_quota_per_hour") == 4);

    // Recovery
    CHECK(config.getMoney("recovery.hysteresis_delta_rupees").toRupees() == 5000.0);
    CHECK(config.getInt("recovery.reroute_max_attempts") == 2);
    CHECK(std::abs(config.getDouble("recovery.processor_radius_km") - 30.0) < 1e-4);
    CHECK(config.getInt("recovery.heartbeat_timeout_ticks") == 3);

    // Kiosk
    CHECK(std::abs(config.getDouble("kiosk.voice_confidence_threshold") - 0.80) < 1e-4);
    CHECK(std::abs(config.getDouble("kiosk.crate_weight_quintals") - 0.25) < 1e-4);
}
