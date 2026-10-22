#include "FakeSlotPort.h"
#include "FakeTruckLockPort.h"
#include "FakeEscrowPort.h"
#include "FakeTelemetrySource.h"
#include "FakeRecoveryContext.h"
#include "../../include/contracts/EventLog.h"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace agro;
using namespace agro::fakes;

class TestWatchdogListener : public ITelemetryWatchdogListener {
public:
    int telemetryCount{0};
    int priceTickCount{0};
    int alertCount{0};

    void onTruckTelemetry(const TelemetryEvent& /*event*/) override {
        telemetryCount++;
    }
    void onMandiPriceTick(const MandiPriceTickEvent& /*event*/) override {
        priceTickCount++;
    }
    void onAnomalyAlert(const AnomalyAlert& /*alert*/) override {
        alertCount++;
    }
};

void testSlotPort() {
    std::cout << "[RUN] Testing FakeSlotPort..." << std::endl;
    FakeSlotPort port;
    AgentId mandi("Azadpur");
    ContractId contract("CTR-001");
    SimTime slotTime(600);

    // 1. Happy path reservation
    auto handleOpt = port.reserveSlot(mandi, slotTime, contract);
    assert(handleOpt.has_value());
    assert(handleOpt->isValid());
    assert(handleOpt->mandiId() == mandi);
    assert(port.activeReservationCount() == 1);
    assert(port.isSlotReserved(mandi, slotTime));

    // 2. Contention on same slot
    ContractId contract2("CTR-002");
    auto handleOpt2 = port.reserveSlot(mandi, slotTime, contract2);
    assert(!handleOpt2.has_value()); // Contended slot rejected

    // 3. Move semantics
    SlotReservationHandle movedHandle = std::move(*handleOpt);
    assert(!handleOpt->isValid());
    assert(movedHandle.isValid());

    // 4. Cancel slot
    bool cancelled = port.cancelSlot(movedHandle);
    assert(cancelled);
    assert(port.activeReservationCount() == 0);
    assert(!port.isSlotReserved(mandi, slotTime));

    // 5. Failure injection
    port.setFailReservations(true);
    auto failedOpt = port.reserveSlot(mandi, slotTime, contract);
    assert(!failedOpt.has_value());
    port.setFailReservations(false);

    std::cout << "  [PASS] FakeSlotPort passed." << std::endl;
}

void testTruckLockPort() {
    std::cout << "[RUN] Testing FakeTruckLockPort..." << std::endl;
    FakeTruckLockPort port;
    AgentId truck1("Truck-101");
    AgentId truck2("Truck-102");
    ContractId contract("CTR-001");

    // 1. Happy path lock
    auto lockOpt = port.lockTruck(truck1, contract);
    assert(lockOpt.has_value());
    assert(lockOpt->isValid());
    assert(port.isTruckLocked(truck1));
    assert(port.activeLockCount() == 1);

    // 2. Lock contended truck
    auto lockOpt2 = port.lockTruck(truck1, ContractId("CTR-002"));
    assert(!lockOpt2.has_value());

    // 3. Rebind truck
    bool rebound = port.rebindTruck(*lockOpt, truck2);
    assert(rebound);
    assert(!port.isTruckLocked(truck1)); // Old truck unlocked
    assert(port.isTruckLocked(truck2)); // New truck locked
    assert(lockOpt->truckId() == truck2);

    // 4. Release truck
    bool released = port.releaseTruck(*lockOpt);
    assert(released);
    assert(port.activeLockCount() == 0);
    assert(!port.isTruckLocked(truck2));

    // 5. Failure injection
    port.setFailLocks(true);
    auto failedLock = port.lockTruck(truck1, contract);
    assert(!failedLock.has_value());
    port.setFailLocks(false);

    std::cout << "  [PASS] FakeTruckLockPort passed." << std::endl;
}

void testEscrowPort() {
    std::cout << "[RUN] Testing FakeEscrowPort..." << std::endl;
    FakeEscrowPort port;
    AgentId farmer("Farmer-01");
    AgentId transporter("Transporter-01");
    ContractId contract("CTR-001");
    Money deposit = Money::fromRupees(10000); // INR 10,000

    // 1. Hold deposit
    auto escrowOpt = port.holdDeposit(farmer, deposit, contract);
    assert(escrowOpt.has_value());
    assert(escrowOpt->isValid());
    assert(port.totalHeld() == deposit);
    assert(port.activeEscrowCount() == 1);

    // 2. Transfer partial freight
    Money freight = Money::fromRupees(4000);
    bool transferred = port.transferEscrow(*escrowOpt, transporter, freight);
    assert(transferred);
    assert(port.totalTransferred() == freight);
    assert(port.totalHeld() == Money::fromRupees(6000));

    // 3. Refund remaining
    Money remaining = Money::fromRupees(6000);
    bool refunded = port.refundEscrow(*escrowOpt, remaining);
    assert(refunded);
    assert(port.totalRefunded() == remaining);
    assert(port.totalHeld().paise == 0);
    assert(port.activeEscrowCount() == 0);

    // 4. Failure injection
    port.setFailHolds(true);
    auto failedHold = port.holdDeposit(farmer, deposit, contract);
    assert(!failedHold.has_value());
    port.setFailHolds(false);

    std::cout << "  [PASS] FakeEscrowPort passed." << std::endl;
}

void testTelemetrySource() {
    std::cout << "[RUN] Testing FakeTelemetrySource..." << std::endl;
    FakeTelemetrySource source;
    TestWatchdogListener listener;
    source.addListener(&listener);

    TelemetryEvent tEvent;
    tEvent.truckId = AgentId("Truck-1");
    tEvent.state = TruckState::EN_ROUTE;
    tEvent.currentQuality = 0.92;
    source.emitTelemetry(tEvent);
    assert(listener.telemetryCount == 1);

    MandiPriceTickEvent pEvent;
    pEvent.mandiId = AgentId("Azadpur");
    pEvent.quotedPrice = Money::fromRupees(2200);
    source.emitMandiPriceTick(pEvent);
    assert(listener.priceTickCount == 1);

    AnomalyAlert alert;
    alert.type = AnomalyType::HighwayStoppage;
    alert.description = "Stall detected on NH44";
    source.emitAnomalyAlert(alert);
    assert(listener.alertCount == 1);

    std::cout << "  [PASS] FakeTelemetrySource passed." << std::endl;
}

void testRecoveryContext() {
    std::cout << "[RUN] Testing FakeRecoveryContext..." << std::endl;
    FakeRecoveryContext ctx;

    // Add candidate trucks
    CandidateTruck t1;
    t1.truckId = AgentId("T1");
    t1.currentNode = "Junction_Kolar";
    t1.capacity = Quintals(50.0);
    ctx.addTruck(t1);

    CandidateTruck t2;
    t2.truckId = AgentId("T2");
    t2.currentNode = "Junction_Kolar";
    t2.capacity = Quintals(20.0);
    ctx.addTruck(t2);

    auto idleTrucks = ctx.findIdleTrucks("Junction_Kolar", Quintals(40.0));
    assert(idleTrucks.size() == 1);
    assert(idleTrucks[0].truckId == AgentId("T1"));

    // Add candidate processors
    CandidateProcessor p1;
    p1.processorId = AgentId("Kissan_Plant_1");
    p1.distanceKm = 25.0;
    p1.currentBidPricePerQuintal = Money::fromRupees(900);
    ctx.addProcessor(p1);

    CandidateProcessor p2;
    p2.processorId = AgentId("Dabur_Plant_2");
    p2.distanceKm = 45.0;
    p2.currentBidPricePerQuintal = Money::fromRupees(950);
    ctx.addProcessor(p2);

    auto nearbyProcessors = ctx.findProcessors("Junction_Kolar", 30.0);
    assert(nearbyProcessors.size() == 1);
    assert(nearbyProcessors[0].processorId == AgentId("Kissan_Plant_1"));

    // Quality prediction decay test:
    // Q(t) = Q0 * exp(-lambda * t * mu_temp)
    // Tomato: lambda = 0.030, mu_temp = 1.2 => effective = 0.036
    // t = 8 hours => Q(8) = exp(-0.036 * 8) = exp(-0.288) approx 0.74976
    CropBatch batch;
    batch.commodity = CommodityType::Tomato;
    batch.decayLambda = 0.030;
    batch.tempMultiplier = 1.2;
    batch.qualityQ0 = 1.0;
    double q8 = ctx.predictQuality(batch, SimTime::fromHours(8.0));
    assert(std::abs(q8 - std::exp(-0.288)) < 1e-4);

    // Stoppage: 15 hours => exp(-0.036 * 15) = exp(-0.54) approx 0.5827 (breaches 0.60 floor!)
    double q15 = ctx.predictQuality(batch, SimTime::fromHours(15.0));
    assert(q15 < 0.60);
    assert(std::abs(q15 - std::exp(-0.54)) < 1e-4);

    std::cout << "  [PASS] FakeRecoveryContext passed." << std::endl;
}

void testEventLog() {
    std::cout << "[RUN] Testing EventLog..." << std::endl;
    auto& log = EventLog::instance();
    log.clear();

    log.append(SimTime(10), EventLevel::INFO, "CONTRACT", "Contract CTR-001 created");
    log.append(SimTime(60), EventLevel::FAULT, "WATCHDOG", "Highway stoppage detected: +7h transit delay");
    log.append(SimTime(65), EventLevel::RECOVERY, "RECOVERY", "Distress auction initiated with nearby plants");

    auto records = log.getRecords();
    assert(records.size() == 3);
    assert(records[1].level == EventLevel::FAULT);
    assert(records[2].level == EventLevel::RECOVERY);

    auto lines = log.getFormattedLines(true);
    assert(lines.size() == 3);
    assert(lines[1].find("[FAULT]") != std::string::npos);

    std::cout << "  [PASS] EventLog passed." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "KisanSetu (MandiFlow) - Phase B0 Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    testSlotPort();
    testTruckLockPort();
    testEscrowPort();
    testTelemetrySource();
    testRecoveryContext();
    testEventLog();

    std::cout << "========================================" << std::endl;
    std::cout << "All Phase B0 Fake & Contract Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
