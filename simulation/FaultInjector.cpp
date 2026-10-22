#include "FaultInjector.h"
#include "../include/contracts/EventLog.h"

namespace agro {

FaultInjector::FaultInjector(uint64_t seed)
    : m_seed(seed), m_rng(seed) {}

void FaultInjector::reset(uint64_t seed) {
    m_seed = seed;
    m_rng.seed(seed);
    m_faults.clear();
}

void FaultInjector::scheduleFault(
    SimTime triggerTime,
    AnomalyType faultType,
    std::string contractId,
    std::string targetAgentId,
    std::string description) {
    
    m_faults.push_back(ScriptedFault{
        triggerTime,
        faultType,
        std::move(contractId),
        std::move(targetAgentId),
        std::move(description),
        false
    });
}

void FaultInjector::loadStandardBenchmarkScenarios() {
    m_faults.clear();

    // Fault 1: Highway Stoppage on NH44 at t = 180 min (3h into trip, adding 7h delay -> Q drops to 0.583 < 0.60)
    scheduleFault(
        SimTime(180),
        AnomalyType::HighwayStoppage,
        "CTR-FAULT-01",
        "Truck-DL01",
        "Highway stoppage on NH44: +7 hours transit delay causing tomato spoilage breach (Q_pred=0.583 < 0.60)."
    );

    // Fault 2: Mandi Glut at Azadpur at t = 240 min (unannounced arrival of 50 trucks drops price by 40% to 1320 < 1800)
    scheduleFault(
        SimTime(240),
        AnomalyType::MandiGlut,
        "CTR-FAULT-02",
        "Azadpur",
        "Supply surge glut at Azadpur Mandi: 50 unannounced arrivals crashed price to INR 1320/q (< reserve INR 1800/q)."
    );

    // Fault 3: Engine Failure on NH48 at t = 300 min (Truck mechanical breakdown)
    scheduleFault(
        SimTime(300),
        AnomalyType::TransporterFailure,
        "CTR-FAULT-03",
        "Truck-DL03",
        "Transporter Truck-DL03 engine breakdown mid-transit: entered FAILED state."
    );
}

std::vector<AnomalyAlert> FaultInjector::pollDueFaults(SimTime currentSimTime) {
    std::vector<AnomalyAlert> triggeredAlerts;

    for (auto& fault : m_faults) {
        if (!fault.triggered && currentSimTime.ticks >= fault.triggerTime.ticks) {
            fault.triggered = true;

            AnomalyAlert alert;
            alert.type = fault.faultType;
            alert.contractId = ContractId(fault.contractId);
            alert.agentId = AgentId(fault.targetAgentId);
            alert.detectedAt = currentSimTime;
            alert.description = fault.description;

            EventLog::instance().append(
                currentSimTime, EventLevel::FAULT, "FAULT_INJECTOR",
                "INJECTING FAULT [seed " + std::to_string(m_seed) + "]: " + fault.description);

            triggeredAlerts.push_back(std::move(alert));
        }
    }

    return triggeredAlerts;
}

} // namespace agro
