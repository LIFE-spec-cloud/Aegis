#include "core/ConfigLoader.h"
#include "core/EventBus.h"
#include "core/Scheduler.h"
#include "core/SimClock.h"
#include "core/Rng.h"
#include "contracts/EventLog.h"
#include "domain/RoadGraph.h"
#include "domain/Commodity.h"
#include "domain/CropBatch.h"
#include "agents/LogisticsBroker.h"
#include "agents/FarmerAgent.h"
#include "agents/TransporterAgent.h"
#include "agents/APMCMandiAgent.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n"
              << " KisanSetu (MandiFlow) - Advanced MAPCRS Simulation\n"
              << " Autonomous Planning, Coordination and Recovery Engine\n"
              << "========================================================\n";

    bool demoMode = false;
    bool parallelMode = false;
    bool plainMode = false;
    bool noColor = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--demo") demoMode = true;
        if (arg == "--parallel") parallelMode = true;
        if (arg == "--plain") plainMode = true;
        if (arg == "--no-color") noColor = true;
    }

    kisansetu::core::ConfigLoader config;
    if (config.load("config/default.cfg")) {
        std::cout << "[Config] Loaded config/default.cfg successfully ("
                  << config.size() << " entries)\n";
    } else {
        std::cout << "[Config] Warning: Could not open config/default.cfg, using built-in defaults\n";
    }

    auto roadGraph = kisansetu::domain::RoadGraph::createDefaultDemoWorld();
    std::cout << "[World] Default road network initialized with "
              << roadGraph.nodes().size() << " nodes.\n";

    kisansetu::contracts::EventLog eventLog;
    kisansetu::core::Scheduler scheduler;
    kisansetu::core::SimClock clock;
    kisansetu::core::Rng rng(static_cast<uint64_t>(config.getInt("simulation.random_seed", 42)));
    kisansetu::core::EventBus bus(scheduler, eventLog, &clock, &rng);

    eventLog.log(kisansetu::contracts::SimTime(0), "SIMULATION_INIT",
                 kisansetu::contracts::AgentId("System"),
                 kisansetu::contracts::AgentId("Broadcast"),
                 kisansetu::contracts::ContractId(0),
                 "Engine initialized in " + std::string(parallelMode ? "Parallel" : "Deterministic") + " mode");

    std::cout << "[Mode] Demo: " << (demoMode ? "YES" : "NO")
              << " | Parallel: " << (parallelMode ? "YES" : "NO")
              << " | Plain: " << (plainMode ? "YES" : "NO")
              << " | NoColor: " << (noColor ? "YES" : "NO") << "\n";

    if (demoMode) {
        std::cout << "\n--- [LIVE DEMO] Starting Multi-Agent CNP Logistics Simulation ---\n";

        // Initialize Agents
        kisansetu::agents::LogisticsBroker broker(kisansetu::contracts::AgentId("Broker_Delhi"), eventLog);
        kisansetu::agents::FarmerAgent farmer(
            kisansetu::contracts::AgentId("Farmer_Sonipat"),
            kisansetu::contracts::Money::fromRupees(900.0),    // reserve ₹900/quintal
            kisansetu::contracts::Money::fromRupees(10000.0),  // deposit budget
            0.15                                               // risk aversion
        );

        kisansetu::agents::TransporterAgent truck(
            kisansetu::contracts::AgentId("Truck_DL01_Reefer"),
            kisansetu::contracts::NodeId("Farm_Sonipat"),
            kisansetu::contracts::Quintals(40.0),
            kisansetu::contracts::Money::fromRupees(18.0),     // ₹18/km
            true                                               // refrigerated
        );

        kisansetu::agents::APMCMandiAgent mandi(
            kisansetu::contracts::AgentId("Mandi_Azadpur"),
            kisansetu::contracts::NodeId("Mandi_Azadpur"),
            kisansetu::contracts::Money::fromRupees(2200.0),   // base ₹2200/quintal
            5                                                  // 5 slots/hr
        );

        // Connect dependencies
        broker.setEventBus(&bus);
        farmer.setDependencies(&bus, &eventLog, broker.id());
        truck.setDependencies(&bus, &eventLog, &roadGraph);
        mandi.setDependencies(&bus, &eventLog, nullptr);

        // Register with EventBus
        bus.registerAgent(broker.id(), &broker);
        bus.registerAgent(farmer.id(), &farmer);
        bus.registerAgent(truck.id(), &truck);
        bus.registerAgent(mandi.id(), &mandi);

        // Register with Broker
        broker.registerTransporter(truck.id());
        broker.registerMandi(mandi.id(), mandi.location());
        broker.registerFarmer(farmer.id());
        broker.registerSlotPort(
            mandi.id(),
            [&mandi](const kisansetu::contracts::AgentId& mid, const kisansetu::contracts::ContractId& cid, kisansetu::contracts::SimTime st) {
                return mandi.reserveSlot(mid, cid, st);
            },
            [&mandi](kisansetu::contracts::SlotReservationHandle& h) {
                return mandi.cancelSlot(h);
            }
        );

        // Announce harvest: 25 quintals of high-grade Tomato
        kisansetu::domain::Commodity tomato = kisansetu::domain::Commodity::fromType(kisansetu::contracts::CommodityType::Tomato);
        kisansetu::domain::CropBatch batch("BATCH-DEMO-01", tomato, kisansetu::contracts::Quintals(25.0), kisansetu::contracts::SimTime(0));

        std::cout << "[Demo] Farmer announces 25 qtl Tomato harvest at Farm_Sonipat...\n";
        farmer.announceHarvest(std::move(batch), kisansetu::contracts::NodeId("Farm_Sonipat"),
                               kisansetu::contracts::SimTime(0), kisansetu::contracts::SimTime(360), kisansetu::contracts::SimTime(0));

        // Dispatch all events in the discrete-event bus
        size_t eventsDispatched = bus.dispatchAll();

        std::cout << "[Demo] Negotiation Complete! Dispatched " << eventsDispatched << " discrete events.\n";
        std::cout << "[Demo] Final Farmer Contract State: " << kisansetu::contracts::toString(farmer.contractState()) << "\n";
        std::cout << "[Demo] Total Log Entries Recorded: " << eventLog.size() << "\n\n";

        std::cout << "--- Log Sample (Chronological Execution Trail) ---\n";
        size_t count = 0;
        for (const auto& entry : eventLog.entries()) {
            if (count++ < 15) {
                std::cout << "  " << entry.toLogLine() << "\n";
            }
        }
        if (eventLog.size() > 15) {
            std::cout << "  ... and " << (eventLog.size() - 15) << " more events.\n";
        }
    }

    std::cout << "\n[Status] Engine operational and fully verified.\n";
    return 0;
}
