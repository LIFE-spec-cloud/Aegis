#include "core/ConfigLoader.h"
#include "contracts/EventLog.h"
#include "domain/RoadGraph.h"
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
    eventLog.log(kisansetu::contracts::SimTime(0), "SIMULATION_INIT",
                 kisansetu::contracts::AgentId("System"),
                 kisansetu::contracts::AgentId("Broadcast"),
                 kisansetu::contracts::ContractId(0),
                 "Engine initialized in " + std::string(parallelMode ? "Parallel" : "Deterministic") + " mode");

    std::cout << "[EventLog] Event count: " << eventLog.size() << "\n";
    std::cout << "[Mode] Demo: " << (demoMode ? "YES" : "NO")
              << " | Parallel: " << (parallelMode ? "YES" : "NO")
              << " | Plain: " << (plainMode ? "YES" : "NO")
              << " | NoColor: " << (noColor ? "YES" : "NO") << "\n";
    std::cout << "[Status] Skeleton ready for Sync 0.\n";

    return 0;
}
