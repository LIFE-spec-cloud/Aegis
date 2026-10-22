#include "../observer/TelemetryWatchdog.h"
#include "../recovery/IFailureRecovery.h"
#include "../recovery/DistressAuctionRecovery.h"
#include "../recovery/DynamicReroutingRecovery.h"
#include "../recovery/PeerHandoffRecovery.h"
#include "../simulation/FaultInjector.h"
#include "../commands/TransitContractCommand.h"
#include "../commands/InvariantChecker.h"
#include "../tests/fakes/FakeTruckLockPort.h"
#include "../tests/fakes/FakeSlotPort.h"
#include "../tests/fakes/FakeEscrowPort.h"
#include "../tests/fakes/FakeRecoveryContext.h"
#include "../include/contracts/EventLog.h"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace agro;
using namespace agro::fakes;

class MockWatchdogListener : public ITelemetryWatchdogListener {
public:
    int truckEvents{0};
    int priceTicks{0};
    int anomalyAlerts{0};
    std::vector<AnomalyAlert> receivedAlerts;

    void onTruckTelemetry(const TelemetryEvent& /*event*/) override {
        truckEvents++;
    }
    void onMandiPriceTick(const MandiPriceTickEvent& /*event*/) override {
        priceTicks++;
    }
    void onAnomalyAlert(const AnomalyAlert& alert) override {
        anomalyAlerts++;
        receivedAlerts.push_back(alert);
    }
};

TransitContractParams createContractParams(const std::string& id, const std::string& mandi, const std::string& truck) {
    TransitContractParams params;
    params.contractId = ContractId(id);
    params.farmerId = AgentId("Farmer-Kisan");
    params.transporterId = AgentId(truck);
    params.mandiId = AgentId(mandi);
    params.slotTime = SimTime(480);
    params.depositAmount = Money::fromRupees(15000);
    params.freightCost = Money::fromRupees(6000);
    params.createdAt = SimTime(60);
    return params;
}

void testObserverPatternNecessity() {
    std::cout << "[RUN] Testing Observer Pattern (TelemetryWatchdog + Listeners)..." << std::endl;
    TelemetryWatchdog watchdog;
    MockWatchdogListener listenerA;
    MockWatchdogListener listenerB;

    watchdog.registerListener(&listenerA);
    watchdog.registerListener(&listenerB);
    assert(watchdog.listenerCount() == 2);

    // Emitting telemetry notifies all registered listeners
    TelemetryEvent tEvent;
    tEvent.truckId = AgentId("Truck-DL01");
    tEvent.state = TruckState::EN_ROUTE;
    tEvent.predictedQualityAtArrival = 0.85; // Healthy
    watchdog.processTruckTelemetry(tEvent);

    assert(listenerA.truckEvents == 1);
    assert(listenerB.truckEvents == 1);
    assert(listenerA.anomalyAlerts == 0);

    // Unregister listenerA; listenerB still receives updates
    watchdog.unregisterListener(&listenerA);
    assert(watchdog.listenerCount() == 1);

    TelemetryEvent tEvent2;
    tEvent2.truckId = AgentId("Truck-DL01");
    tEvent2.state = TruckState::FAILED;
    watchdog.processTruckTelemetry(tEvent2);

    assert(listenerA.truckEvents == 1); // Unchanged
    assert(listenerB.truckEvents == 2);
    assert(listenerB.anomalyAlerts == 1);
    assert(listenerB.receivedAlerts.front().type == AnomalyType::TransporterFailure);

    std::cout << "  [PASS] Observer Pattern decoupled listeners cleanly." << std::endl;
}

void testScenario1_HighwayStoppage_DistressAuction() {
    std::cout << "[RUN] Testing Scenario 1: Highway Stoppage -> Distress Auction Recovery..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;
    FakeRecoveryContext context;

    // Scripted candidate processing plants:
    // Plant 1: 25 km away, bids 950 INR/q
    CandidateProcessor p1;
    p1.processorId = AgentId("Kissan_Plant_1");
    p1.name = "Kissan Food Processing";
    p1.distanceKm = 25.0;
    p1.currentBidPricePerQuintal = Money::fromRupees(950);
    context.addProcessor(p1);

    // Plant 2: 15 km away, bids 1050 INR/q (Winner: within 30 km and highest net bid)
    CandidateProcessor p2;
    p2.processorId = AgentId("Tropicana_Plant_2");
    p2.name = "Tropicana Processing Hub";
    p2.distanceKm = 15.0;
    p2.currentBidPricePerQuintal = Money::fromRupees(1050);
    context.addProcessor(p2);

    // Plant 3: 45 km away, bids 1200 INR/q (Beyond 30 km radius threshold -> must NOT win)
    CandidateProcessor p3;
    p3.processorId = AgentId("Dabur_Plant_3");
    p3.name = "Dabur Distant Plant";
    p3.distanceKm = 45.0;
    p3.currentBidPricePerQuintal = Money::fromRupees(1200);
    context.addProcessor(p3);

    auto params = createContractParams("CTR-SCENARIO-01", "Azadpur", "Truck-DL01");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);
    assert(cmd.execute() == CommandStatus::Success);
    assert(slotPort.activeReservationCount() == 1);
    assert(truckPort.activeLockCount() == 1);

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Inject Highway stoppage: +7h delay drops Q from 0.75 to 0.583 (< 0.60 mandi floor)
    CropBatch batch;
    batch.commodity = CommodityType::Tomato;
    batch.decayLambda = 0.030;
    batch.tempMultiplier = 1.2;
    double predictedQ = context.predictQuality(batch, SimTime::fromHours(15.0));
    assert(predictedQ < 0.60); // Math calibration check

    AnomalyAlert alert;
    alert.type = AnomalyType::HighwayStoppage;
    alert.contractId = cmd.contractId();
    alert.description = "Highway stoppage +7h delay: Q=" + std::to_string(predictedQ) + " < 0.60 floor.";

    // Execute DistressAuctionRecovery
    DistressAuctionRecovery recovery(30.0); // 30 km radius
    auto result = recovery.recover(cmd, context, alert, SimTime(240));

    assert(result.success);
    assert(result.newDestinationOrPeer == AgentId("Tropicana_Plant_2"));
    assert(result.finalPriceOrCost == Money::fromRupees(1050));
    assert(cmd.mandiId() == AgentId("Tropicana_Plant_2"));

    // Key invariant assertion: Mandi gate slot was released upon diversion to processor!
    assert(slotPort.activeReservationCount() == 0);
    assert(truckPort.activeLockCount() == 1); // Truck remains locked for transport
    assert(!cmd.hasSlot()); // Command no longer holds mandi slot
    assert(cmd.hasTruckLock());

    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Complete contract at processor
    assert(cmd.complete(SimTime(360)));
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);
    assert(truckPort.activeLockCount() == 0);
    assert(slotPort.activeReservationCount() == 0);

    std::cout << "  [PASS] Scenario 1: Highway stoppage diverted to 15km processor at INR 1050/q with 0 leaked slots." << std::endl;
}

void testScenario2_MandiGlut_DynamicRerouting() {
    std::cout << "[RUN] Testing Scenario 2: Mandi Glut -> Dynamic Rerouting Recovery..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;
    FakeRecoveryContext context;

    // Scripted alternative mandis:
    // Ghazipur: net margin gain = 16,500 (> delta 5,000 threshold) -> Winner!
    CandidateMandi m1;
    m1.mandiId = AgentId("Ghazipur");
    m1.name = "Ghazipur APMC";
    m1.quotedPrice = Money::fromRupees(2100);
    m1.expectedNetMargin = Money::fromRupees(16500);
    m1.nextSlotTime = SimTime(540);
    context.addMandi(m1);

    // Kolar: net margin = 3,000 (< delta 5,000 threshold)
    CandidateMandi m2;
    m2.mandiId = AgentId("Kolar");
    m2.name = "Kolar APMC";
    m2.quotedPrice = Money::fromRupees(1500);
    m2.expectedNetMargin = Money::fromRupees(3000);
    m2.nextSlotTime = SimTime(600);
    context.addMandi(m2);

    // Vashi: net margin = 18,000 (> delta 5,000 threshold)
    CandidateMandi m3;
    m3.mandiId = AgentId("Vashi");
    m3.name = "Vashi APMC";
    m3.quotedPrice = Money::fromRupees(2300);
    m3.expectedNetMargin = Money::fromRupees(18000);
    m3.nextSlotTime = SimTime(580);
    context.addMandi(m3);

    auto params = createContractParams("CTR-SCENARIO-02", "Azadpur", "Truck-DL02");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);
    assert(cmd.execute() == CommandStatus::Success);
    assert(slotPort.isSlotReserved(AgentId("Azadpur"), SimTime(480)));

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Azadpur prices crash by 40% (₹2200 -> ₹1320 < ₹1800 reserve)
    AnomalyAlert alert;
    alert.type = AnomalyType::MandiGlut;
    alert.contractId = cmd.contractId();
    alert.description = "Azadpur glut: price crashed to 1320 INR/q below farmer reserve 1800 INR/q.";

    // Execute DynamicReroutingRecovery (hysteresis delta = ₹5,000, max reroutes = 2)
    DynamicReroutingRecovery recovery(Money::fromRupees(5000), 2);
    auto result = recovery.recover(cmd, context, alert, SimTime(200));

    assert(result.success);
    assert(result.newDestinationOrPeer == AgentId("Vashi"));
    assert(cmd.mandiId() == AgentId("Vashi"));
    assert(cmd.slotTime() == SimTime(580));

    // Old slot at Azadpur cancelled, new slot at Vashi reserved
    assert(!slotPort.isSlotReserved(AgentId("Azadpur"), SimTime(480)));
    assert(slotPort.isSlotReserved(AgentId("Vashi"), SimTime(580)));
    assert(slotPort.activeReservationCount() == 1);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Test Bounded Attempts:
    // Attempt 2 succeeds (reroutes to Ghazipur, as Vashi is now excluded)
    auto result2 = recovery.recover(cmd, context, alert, SimTime(250));
    assert(result2.success);
    assert(result2.newDestinationOrPeer == AgentId("Ghazipur"));
    assert(cmd.mandiId() == AgentId("Ghazipur"));
    assert(recovery.rerouteCount(cmd.contractId().str()) == 2);

    // Attempt 3 must be REJECTED (max 2 reroutes reached)
    auto result3 = recovery.recover(cmd, context, alert, SimTime(300));
    assert(!result3.success);
    assert(result3.actionSummary.find("reached maximum reroute attempts") != std::string::npos);

    // Complete delivery at Ghazipur
    assert(cmd.complete(SimTime(600)));
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    std::cout << "  [PASS] Scenario 2: Mandi glut rerouted to Ghazipur with hysteresis and bounded attempts." << std::endl;
}

void testScenario3_TransporterBreakdown_PeerHandoff() {
    std::cout << "[RUN] Testing Scenario 3: Engine Failure -> Peer Handoff Recovery..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;
    FakeRecoveryContext context;

    // Scripted candidate peer truck:
    CandidateTruck peer;
    peer.truckId = AgentId("Truck-DL99");
    peer.currentNode = "NH44_Junction";
    peer.capacity = Quintals(50.0);
    context.addTruck(peer);

    auto params = createContractParams("CTR-SCENARIO-03", "Azadpur", "Truck-DL03");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);
    assert(cmd.execute() == CommandStatus::Success);
    assert(truckPort.isTruckLocked(AgentId("Truck-DL03")));

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Injected Engine breakdown
    AnomalyAlert alert;
    alert.type = AnomalyType::TransporterFailure;
    alert.contractId = cmd.contractId();
    alert.description = "Truck-DL03 engine failure mid-transit.";

    // Execute PeerHandoffRecovery
    PeerHandoffRecovery recovery(Quintals(50.0));
    auto result = recovery.recover(cmd, context, alert, SimTime(300));

    assert(result.success);
    assert(result.newDestinationOrPeer == AgentId("Truck-DL99"));
    assert(cmd.transporterId() == AgentId("Truck-DL99"));

    // Old failed truck unlocked, new peer truck locked
    assert(!truckPort.isTruckLocked(AgentId("Truck-DL03")));
    assert(truckPort.isTruckLocked(AgentId("Truck-DL99")));
    assert(truckPort.activeLockCount() == 1);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Complete contract with peer truck
    assert(cmd.complete(SimTime(500)));
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);
    assert(truckPort.activeLockCount() == 0);

    std::cout << "  [PASS] Scenario 3: Transporter breakdown seamlessly rebound to peer truck with zero leaks." << std::endl;
}

void testScenario4_PeerHandoff_FallbackChain() {
    std::cout << "[RUN] Testing Scenario 4: Peer Handoff Fallback Chain (No Idle Truck -> Distress Sale)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;
    FakeRecoveryContext context;

    // No idle trucks in context! Only processors:
    CandidateProcessor fallbackPlant;
    fallbackPlant.processorId = AgentId("Emergency_Cold_Storage_Plant");
    fallbackPlant.name = "Emergency Plant";
    fallbackPlant.distanceKm = 10.0;
    fallbackPlant.currentBidPricePerQuintal = Money::fromRupees(800);
    context.addProcessor(fallbackPlant);

    auto params = createContractParams("CTR-SCENARIO-04", "Azadpur", "Truck-DL04");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);
    assert(cmd.execute() == CommandStatus::Success);

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Transporter failure
    AnomalyAlert alert;
    alert.type = AnomalyType::TransporterFailure;
    alert.contractId = cmd.contractId();
    alert.description = "Truck-DL04 breakdown with no idle peer trucks in range.";

    // Set fallback recovery to DistressAuctionRecovery
    auto fallback = std::make_shared<DistressAuctionRecovery>(30.0);
    PeerHandoffRecovery recovery(Quintals(50.0), fallback);

    auto result = recovery.recover(cmd, context, alert, SimTime(310));
    assert(result.success);
    assert(result.strategyName == "DistressAuctionRecovery"); // Activated fallback!
    assert(result.newDestinationOrPeer == AgentId("Emergency_Cold_Storage_Plant"));
    assert(slotPort.activeReservationCount() == 0); // Slot released
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    std::cout << "  [PASS] Scenario 4: Fallback chain successfully engaged distress auction." << std::endl;
}

void testFaultInjector_DeterminismSeed42() {
    std::cout << "[RUN] Testing FaultInjector Determinism with Seed 42..." << std::endl;
    FaultInjector injector1(42);
    injector1.loadStandardBenchmarkScenarios();

    FaultInjector injector2(42);
    injector2.loadStandardBenchmarkScenarios();

    // Poll at t = 100, 180, 240, 300
    std::vector<SimTime> checkTimes = {SimTime(100), SimTime(180), SimTime(240), SimTime(300)};

    for (auto t : checkTimes) {
        auto alerts1 = injector1.pollDueFaults(t);
        auto alerts2 = injector2.pollDueFaults(t);

        assert(alerts1.size() == alerts2.size());
        for (size_t i = 0; i < alerts1.size(); ++i) {
            assert(alerts1[i].type == alerts2[i].type);
            assert(alerts1[i].contractId == alerts2[i].contractId);
            assert(alerts1[i].agentId == alerts2[i].agentId);
            assert(alerts1[i].description == alerts2[i].description);
            assert(alerts1[i].detectedAt == alerts2[i].detectedAt);
        }
    }

    std::cout << "  [PASS] FaultInjector with seed 42 executed identically across multiple runs." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "KisanSetu (MandiFlow) - Phase B2 Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    testObserverPatternNecessity();
    testScenario1_HighwayStoppage_DistressAuction();
    testScenario2_MandiGlut_DynamicRerouting();
    testScenario3_TransporterBreakdown_PeerHandoff();
    testScenario4_PeerHandoff_FallbackChain();
    testFaultInjector_DeterminismSeed42();

    std::cout << "========================================" << std::endl;
    std::cout << "All Phase B2 Telemetry & Recovery Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
