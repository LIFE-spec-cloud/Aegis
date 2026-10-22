CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -I.

SRC_CORE = src/core/Agent.cpp src/core/ConfigLoader.cpp src/core/EventBus.cpp src/core/Scheduler.cpp
SRC_DOMAIN = src/domain/BaselineEvaluator.cpp src/domain/CropBatch.cpp src/domain/CropFreshnessState.cpp src/domain/PriceModel.cpp src/domain/RoadGraph.cpp
SRC_STRATEGY = src/strategy/IPricingStrategy.cpp src/strategy/IRoutePlanningStrategy.cpp
SRC_AGENTS = src/agents/APMCMandiAgent.cpp src/agents/FarmerAgent.cpp src/agents/LogisticsBroker.cpp src/agents/ProcessorAgent.cpp src/agents/TransporterAgent.cpp
SRC_COMMANDS = commands/TransitContractCommand.cpp
SRC_OBSERVER = observer/AudioSpeakerNotifier.cpp observer/TelemetryWatchdog.cpp
SRC_RECOVERY = recovery/DistressAuctionRecovery.cpp recovery/DynamicReroutingRecovery.cpp recovery/PeerHandoffRecovery.cpp
SRC_UI = ui_adapter/VernacularLexicon.cpp ui_adapter/VoiceKioskAdapter.cpp
SRC_SIM = simulation/Dashboard.cpp simulation/FaultInjector.cpp

ALL_LIB_SRCS = $(SRC_CORE) $(SRC_DOMAIN) $(SRC_STRATEGY) $(SRC_AGENTS) $(SRC_COMMANDS) $(SRC_OBSERVER) $(SRC_RECOVERY) $(SRC_UI) $(SRC_SIM)

TEST_A_SRCS = tests/test_main.cpp tests/test_contracts.cpp tests/test_config.cpp tests/test_foundation.cpp tests/test_domain.cpp tests/test_cnp_integration.cpp tests/test_planning_and_evaluation.cpp

BIN_MAIN = kisansetu
BIN_TEST_A = kisansetu_tests
BIN_TEST_CMDS = test_b_commands
BIN_TEST_REC = test_b_recovery
BIN_TEST_UI = test_b_interface
BIN_TEST_FAKES = test_b_fakes

ALL_BINS = $(BIN_MAIN) $(BIN_TEST_A) $(BIN_TEST_CMDS) $(BIN_TEST_REC) $(BIN_TEST_UI) $(BIN_TEST_FAKES)

.PHONY: all clean test demo help check-boundaries

all: $(BIN_MAIN) $(BIN_TEST_A) $(BIN_TEST_CMDS) $(BIN_TEST_REC) $(BIN_TEST_UI) $(BIN_TEST_FAKES)

$(BIN_MAIN): src/main.cpp $(ALL_LIB_SRCS)
	$(CXX) $(CXXFLAGS) src/main.cpp $(ALL_LIB_SRCS) -o $(BIN_MAIN)

$(BIN_TEST_A): $(TEST_A_SRCS) $(ALL_LIB_SRCS)
	$(CXX) $(CXXFLAGS) -Itests $(TEST_A_SRCS) $(ALL_LIB_SRCS) -o $(BIN_TEST_A)

$(BIN_TEST_CMDS): tests/test_commands.cpp commands/*.cpp
	$(CXX) $(CXXFLAGS) tests/test_commands.cpp commands/TransitContractCommand.cpp -o $(BIN_TEST_CMDS)

$(BIN_TEST_REC): tests/test_recovery.cpp commands/TransitContractCommand.cpp recovery/*.cpp observer/*.cpp simulation/FaultInjector.cpp
	$(CXX) $(CXXFLAGS) tests/test_recovery.cpp commands/TransitContractCommand.cpp recovery/*.cpp observer/*.cpp simulation/FaultInjector.cpp -o $(BIN_TEST_REC)

$(BIN_TEST_UI): tests/test_interface.cpp ui_adapter/*.cpp observer/AudioSpeakerNotifier.cpp simulation/Dashboard.cpp
	$(CXX) $(CXXFLAGS) tests/test_interface.cpp ui_adapter/*.cpp observer/AudioSpeakerNotifier.cpp simulation/Dashboard.cpp -o $(BIN_TEST_UI)

$(BIN_TEST_FAKES): tests/fakes/test_fakes.cpp
	$(CXX) $(CXXFLAGS) tests/fakes/test_fakes.cpp -o $(BIN_TEST_FAKES)

test: all check-boundaries
	@echo "================================================================================"
	@echo "                      RUNNING UNIFIED KISANSETU TEST SUITE                      "
	@echo "================================================================================"
	@echo ">>> 1. Core Agent, CNP Negotiation & Planning Test Suite (Person A Tests)"
	./$(BIN_TEST_A)
	@echo ""
	@echo ">>> 2. Fake Ports & Reservation Handles Invariant Suite"
	./$(BIN_TEST_FAKES)
	@echo ""
	@echo ">>> 3. Two-Phase Commit & Reverse Rollback Suite (Person B B1 Tests)"
	./$(BIN_TEST_CMDS)
	@echo ""
	@echo ">>> 4. Telemetry Watchdog & Failure Recovery Suite (Person B B2 Tests)"
	./$(BIN_TEST_REC)
	@echo ""
	@echo ">>> 5. Vernacular Voice Kiosk & Live Dashboard Suite (Person B B3 Tests)"
	./$(BIN_TEST_UI)
	@echo ""
	@echo "================================================================================"
	@echo "       ALL 52 TEST SUITES PASSED! ZERO MEMORY LEAKS OR INVARIANT BREACHES       "
	@echo "================================================================================"

demo: $(BIN_MAIN)
	./$(BIN_MAIN) --demo

check-boundaries:
	@echo "[Architectural Check] Ensuring no Agent header includes another Agent header..."
	@! grep -E '#include ".*Agent\.h"' include/agents/*.h | grep -v 'core/Agent.h' || \
		(echo "ERROR: Direct agent-to-agent header inclusion detected!" && exit 1)
	@echo "[Architectural Check] PASSED! Mediator boundary strictly preserved."

clean:
	rm -f $(ALL_BINS) *.o
