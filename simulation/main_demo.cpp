#include "../observer/TelemetryWatchdog.h"
#include "../observer/AudioSpeakerNotifier.h"
#include "../recovery/DistressAuctionRecovery.h"
#include "../recovery/DynamicReroutingRecovery.h"
#include "../recovery/PeerHandoffRecovery.h"
#include "../simulation/FaultInjector.h"
#include "../simulation/Dashboard.h"
#include "../ui_adapter/VoiceKioskAdapter.h"
#include "../ui_adapter/ProduceStatusCard.h"
#include "../commands/TransitContractCommand.h"
#include "../commands/InvariantChecker.h"
#include "../tests/fakes/FakeTruckLockPort.h"
#include "../tests/fakes/FakeSlotPort.h"
#include "../tests/fakes/FakeEscrowPort.h"
#include "../tests/fakes/FakeRecoveryContext.h"
#include "../include/contracts/EventLog.h"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

using namespace agro;
using namespace agro::fakes;

int main(int argc, char* argv[]) {
    DashboardMode mode = DashboardMode::AnsiColor;
    bool stepMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--plain") mode = DashboardMode::Plain;
        else if (arg == "--no-color") mode = DashboardMode::NoColor;
        else if (arg == "--step") stepMode = true;
    }

    std::cout << "================================================================================" << std::endl;
    std::cout << "   KISANSETU (MANDIFLOW) MULTI-AGENT LOGISTICS & RECOVERY ENGINE" << std::endl;
    std::cout << "   Course Project Demo: Fault-Tolerant Recovery & Accessible Farmer UI" << std::endl;
    std::cout << "================================================================================" << std::endl;

    // 1. Initialize Ports & Shared Context
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;
    FakeRecoveryContext recoveryContext;

    // Seed candidate processors and alternate mandis
    CandidateProcessor p1;
    p1.processorId = AgentId("Tropicana_Plant_2");
    p1.name = "Tropicana Processing Hub";
    p1.distanceKm = 15.0;
    p1.currentBidPricePerQuintal = Money::fromRupees(1050);
    recoveryContext.addProcessor(p1);

    CandidateMandi m1;
    m1.mandiId = AgentId("Ghazipur");
    m1.name = "Ghazipur APMC";
    m1.quotedPrice = Money::fromRupees(2100);
    m1.expectedNetMargin = Money::fromRupees(16500);
    m1.nextSlotTime = SimTime(540);
    recoveryContext.addMandi(m1);

    CandidateTruck idleTruck;
    idleTruck.truckId = AgentId("Truck-DL99");
    idleTruck.currentNode = "NH44_Junction";
    idleTruck.capacity = Quintals(50.0);
    recoveryContext.addTruck(idleTruck);

    // 2. Initialize Watchdog, Speaker Notifier, and UI
    TelemetryWatchdog watchdog;
    AudioSpeakerNotifier speakerNotifier(true);
    watchdog.registerListener(&speakerNotifier);

    Dashboard dashboard(mode);
    dashboard.setSimClock(SimTime(60), 42);

    VoiceKioskAdapter voiceAdapter(0.80);

    // 3. STEP 1: Farmer Ramesh uses Vernacular Voice Kiosk
    std::cout << "\n>>> [STEP 1: VERNACULAR VOICE KIOSK INTERACTION] <<<" << std::endl;
    InputEvent voiceEvent;
    voiceEvent.type = InputEventType::VoiceText;
    voiceEvent.payload = "50 crates tamatar bechna hai";
    voiceEvent.sourceFarmer = AgentId("Farmer-Ramesh");
    voiceEvent.timestamp = SimTime(60);

    std::cout << "Farmer Ramesh speaks: \"" << voiceEvent.payload << "\"" << std::endl;
    auto parseResult = voiceAdapter.parse(voiceEvent);

    assert(parseResult.confirmationCard.has_value());
    std::cout << "\n[KIOSK DISPLAY]: " << parseResult.confirmationCard->title << std::endl;
    std::cout << "Prompt: " << parseResult.confirmationCard->promptHinglish << std::endl;
    std::cout << "Details: " << parseResult.confirmationCard->summaryDetails << std::endl;

    // Display produce status card
    auto statusCard = ProduceStatusCard::fromQuality(CommodityType::Tomato, 0.95);
    std::cout << "\n" << statusCard.format(mode == DashboardMode::Plain) << "\n" << std::endl;

    // Farmer taps Green Tick
    std::cout << "Farmer taps [GREEN TICK] button on kiosk..." << std::endl;
    InputEvent confirmEvent;
    confirmEvent.type = InputEventType::ButtonTap;
    confirmEvent.payload = "GREEN_TICK";
    confirmEvent.sourceFarmer = AgentId("Farmer-Ramesh");
    auto confirmResult = voiceAdapter.parse(confirmEvent);
    assert(confirmResult.command.has_value());

    // 4. STEP 2: 2-Phase Commit Execution
    std::cout << "\n>>> [STEP 2: TWO-PHASE COMMIT EXECUTION] <<<" << std::endl;
    TransitContractParams params;
    params.contractId = ContractId("CTR-DEMO-01");
    params.farmerId = AgentId("Farmer-Ramesh");
    params.transporterId = AgentId("Truck-DL01");
    params.mandiId = AgentId("Azadpur");
    params.slotTime = SimTime(480);
    params.depositAmount = Money::fromRupees(12000);
    params.freightCost = Money::fromRupees(5000);
    params.createdAt = SimTime(60);

    TransitContractCommand contract(params, truckPort, slotPort, escrowPort);
    assert(contract.execute() == CommandStatus::Success);
    std::cout << "Contract " << contract.contractId().value() 
              << " COMMITTED (Truck locked, Azadpur slot reserved, Escrow held)." << std::endl;

    std::vector<const TransitContractCommand*> activeContracts = {&contract};
    assert(InvariantChecker::verify(activeContracts, truckPort, slotPort, escrowPort).passed);

    // Update Dashboard Initial State
    TruckLaneView tView;
    tView.truckId = AgentId("Truck-DL01");
    tView.route = "Kolar->Azadpur";
    tView.progressPercent = 15.0;
    tView.state = TruckState::EN_ROUTE;
    tView.cargoSummary = "12.5q Tomato";
    tView.eta = SimTime(480);
    tView.quality = 0.95;
    dashboard.updateTruck(tView);

    MandiTickerView mAzadpur;
    mAzadpur.mandiId = AgentId("Azadpur");
    mAzadpur.name = "Azadpur";
    mAzadpur.currentPrice = Money::fromRupees(2200);
    mAzadpur.previousPrice = Money::fromRupees(2200);
    mAzadpur.occupiedSlots = 3;
    mAzadpur.capacity = 4;
    dashboard.updateMandi(mAzadpur);

    dashboard.setSimClock(SimTime(120), 42);
    dashboard.display();

    if (stepMode) {
        std::cout << "\n[Press Enter to trigger Fault Scenario 1: Highway Stoppage]...";
        std::cin.get();
    }

    // 5. STEP 3: Fault 1 - Highway Stoppage & Distress Auction Recovery
    std::cout << "\n>>> [STEP 3: INJECTING FAULT 1 - HIGHWAY STOPPAGE ON NH44] <<<" << std::endl;
    dashboard.setSimClock(SimTime(180), 42);

    // Stall adds 7 hours; Q drops to 0.583 < 0.60 floor
    TelemetryEvent tEvent;
    tEvent.truckId = AgentId("Truck-DL01");
    tEvent.contractId = contract.contractId();
    tEvent.state = TruckState::STALLED;
    tEvent.currentSimTime = SimTime(180);
    tEvent.predictedQualityAtArrival = 0.583; // Below 0.60 floor!
    watchdog.processTruckTelemetry(tEvent);

    // Execute Distress Auction Recovery
    DistressAuctionRecovery distressRecovery(30.0);
    AnomalyAlert alertStall;
    alertStall.type = AnomalyType::HighwayStoppage;
    alertStall.contractId = contract.contractId();
    alertStall.description = "Predicted quality breach Q=0.583 < 0.60 floor.";
    auto r1 = distressRecovery.recover(contract, recoveryContext, alertStall, SimTime(185));

    assert(r1.success);
    assert(slotPort.activeReservationCount() == 0); // Slot released!

    tView.state = TruckState::EN_ROUTE;
    tView.route = "NH44->Tropicana";
    tView.progressPercent = 65.0;
    tView.quality = 0.583;
    dashboard.updateTruck(tView);
    dashboard.display();

    assert(InvariantChecker::verify(activeContracts, truckPort, slotPort, escrowPort).passed);

    // Final Contract Completion
    std::cout << "\n>>> [FINAL DELIVERY & SETTLEMENT] <<<" << std::endl;
    dashboard.setSimClock(SimTime(360), 42);
    assert(contract.complete(SimTime(360)));

    tView.progressPercent = 100.0;
    tView.state = TruckState::IDLE;
    dashboard.updateTruck(tView);
    dashboard.display();

    // Final Invariant Check
    auto finalAudit = InvariantChecker::verify(activeContracts, truckPort, slotPort, escrowPort);
    assert(finalAudit.passed);
    std::cout << "\n[INVARIANT CHECK]: " << finalAudit.diagnosticMessage << std::endl;
    std::cout << "All resources freed: 0 leaked truck locks, 0 leaked gate slots, 0 leaked escrow deposits." << std::endl;
    std::cout << "\nDemo successfully finished! System is defense-ready." << std::endl;
    return 0;
}
