#pragma once

#include "../include/contracts/ITelemetry.h"
#include <vector>
#include <random>
#include <string>

namespace agro {

struct ScriptedFault {
    SimTime triggerTime;
    AnomalyType faultType;
    std::string contractId;
    std::string targetAgentId;
    std::string description;
    bool triggered{false};
};

class FaultInjector {
public:
    explicit FaultInjector(uint64_t seed = 42);

    void reset(uint64_t seed = 42);

    // Schedule a scripted deterministic fault
    void scheduleFault(
        SimTime triggerTime,
        AnomalyType faultType,
        std::string contractId,
        std::string targetAgentId,
        std::string description);

    // Load the three standard benchmark course project fault scenarios
    void loadStandardBenchmarkScenarios();

    // Check simulation clock and trigger any due faults
    [[nodiscard]] std::vector<AnomalyAlert> pollDueFaults(SimTime currentSimTime);

    [[nodiscard]] uint64_t seed() const noexcept { return m_seed; }
    [[nodiscard]] const std::vector<ScriptedFault>& scheduledFaults() const noexcept { return m_faults; }

private:
    uint64_t m_seed{42};
    std::mt19937_64 m_rng;
    std::vector<ScriptedFault> m_faults;
};

} // namespace agro
