#include "../commands/TransitContractCommand.h"
#include "../commands/InvariantChecker.h"
#include "../tests/fakes/FakeTruckLockPort.h"
#include "../tests/fakes/FakeSlotPort.h"
#include "../tests/fakes/FakeEscrowPort.h"
#include "../include/contracts/EventLog.h"

#include <iostream>
#include <cassert>
#include <vector>

using namespace agro;
using namespace agro::fakes;

TransitContractParams createDefaultParams(const std::string& id = "CTR-101") {
    TransitContractParams params;
    params.contractId = ContractId(id);
    params.farmerId = AgentId("Farmer-Ramesh");
    params.transporterId = AgentId("Truck-DL01");
    params.mandiId = AgentId("Azadpur");
    params.slotTime = SimTime(480); // 8:00 AM
    params.depositAmount = Money::fromRupees(12000);
    params.freightCost = Money::fromRupees(5000);
    params.createdAt = SimTime(60);
    return params;
}

void testHappyPath2PC() {
    std::cout << "[RUN] Testing 2PC Happy Path (CREATED -> PREPARING -> COMMITTED -> COMPLETED)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    auto params = createDefaultParams("CTR-HP-01");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    assert(cmd.state() == ContractState::CREATED);
    assert(cmd.version() == 1);

    // Initial invariant check
    std::vector<const TransitContractCommand*> contracts = {&cmd};
    auto invRes = InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort);
    assert(invRes.passed);

    // Execute 2-Phase Commit
    CommandStatus status = cmd.execute();
    assert(status == CommandStatus::Success);
    assert(cmd.state() == ContractState::COMMITTED);
    assert(cmd.version() == 2);
    assert(cmd.hasTruckLock());
    assert(cmd.hasSlot());
    assert(cmd.hasEscrow());

    // Invariant check: all 3 resources held
    invRes = InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort);
    assert(invRes.passed);
    assert(truckPort.activeLockCount() == 1);
    assert(slotPort.activeReservationCount() == 1);
    assert(escrowPort.activeEscrowCount() == 1);

    // Complete contract on delivery
    bool completed = cmd.complete(SimTime(600));
    assert(completed);
    assert(cmd.state() == ContractState::COMPLETED);
    assert(!cmd.hasTruckLock());
    assert(!cmd.hasSlot());
    assert(!cmd.hasEscrow());

    // Invariant check: zero leaked resources
    invRes = InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort);
    assert(invRes.passed);
    assert(truckPort.activeLockCount() == 0);
    assert(slotPort.activeReservationCount() == 0);
    assert(escrowPort.activeEscrowCount() == 0);
    assert(escrowPort.totalTransferred() == params.freightCost);
    assert(escrowPort.totalRefunded() == (params.depositAmount - params.freightCost));

    std::cout << "  [PASS] 2PC Happy Path passed." << std::endl;
}

void testFailureAtPrepareStep1_TruckLock() {
    std::cout << "[RUN] Testing 2PC Failure at Step 1 (Truck Lock fails)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    truckPort.setFailLocks(true); // Inject failure

    auto params = createDefaultParams("CTR-FAIL-STEP1");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    CommandStatus status = cmd.execute();
    assert(status == CommandStatus::Failed);
    assert(cmd.state() == ContractState::ABORTED);
    assert(cmd.isTerminal());

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    auto invRes = InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort);
    assert(invRes.passed);
    assert(truckPort.activeLockCount() == 0);
    assert(slotPort.activeReservationCount() == 0);
    assert(escrowPort.activeEscrowCount() == 0);

    std::cout << "  [PASS] Step 1 failure cleanly aborted with 0 leaked locks." << std::endl;
}

void testFailureAtPrepareStep2_SlotReservation() {
    std::cout << "[RUN] Testing 2PC Failure at Step 2 (Slot Reservation fails)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    slotPort.setFailReservations(true); // Inject failure on slot

    auto params = createDefaultParams("CTR-FAIL-STEP2");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    CommandStatus status = cmd.execute();
    assert(status == CommandStatus::Failed);
    assert(cmd.state() == ContractState::ABORTED);
    assert(cmd.isTerminal());

    // Ensure truck lock acquired in Step 1 was rolled back!
    assert(!cmd.hasTruckLock());
    assert(truckPort.activeLockCount() == 0);

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    auto invRes = InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort);
    assert(invRes.passed);

    std::cout << "  [PASS] Step 2 failure rolled back Step 1 with 0 leaked locks." << std::endl;
}

void testFailureAtPrepareStep3_EscrowHold() {
    std::cout << "[RUN] Testing 2PC Failure at Step 3 (Escrow Hold fails)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    escrowPort.setFailHolds(true); // Inject failure on escrow

    auto params = createDefaultParams("CTR-FAIL-STEP3");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    CommandStatus status = cmd.execute();
    assert(status == CommandStatus::Failed);
    assert(cmd.state() == ContractState::ABORTED);
    assert(cmd.isTerminal());

    // Ensure both Step 2 (Slot) and Step 1 (Truck) were rolled back in reverse order!
    assert(!cmd.hasSlot());
    assert(!cmd.hasTruckLock());
    assert(slotPort.activeReservationCount() == 0);
    assert(truckPort.activeLockCount() == 0);

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    auto invRes = InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort);
    assert(invRes.passed);

    std::cout << "  [PASS] Step 3 failure rolled back Steps 2 and 1 in reverse order." << std::endl;
}

void testIdempotentCompensation() {
    std::cout << "[RUN] Testing Idempotent compensate()..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    auto params = createDefaultParams("CTR-IDEMP-01");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    assert(cmd.execute() == CommandStatus::Success);
    assert(cmd.state() == ContractState::COMMITTED);

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Call compensate() first time
    CommandStatus compStatus1 = cmd.compensate();
    assert(compStatus1 == CommandStatus::Success);
    assert(cmd.state() == ContractState::COMPENSATED);
    assert(cmd.isTerminal());
    assert(!cmd.hasTruckLock());
    assert(!cmd.hasSlot());
    assert(!cmd.hasEscrow());

    assert(truckPort.activeLockCount() == 0);
    assert(slotPort.activeReservationCount() == 0);
    assert(escrowPort.activeEscrowCount() == 0);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    uint64_t versionAfterFirstComp = cmd.version();

    // Call compensate() a SECOND time (Idempotency test)
    CommandStatus compStatus2 = cmd.compensate();
    assert(compStatus2 == CommandStatus::Success);
    assert(cmd.state() == ContractState::COMPENSATED);
    assert(cmd.version() == versionAfterFirstComp); // Version unchanged on duplicate call
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    std::cout << "  [PASS] Idempotent compensation verified across multiple calls." << std::endl;
}

void testRebindSlot_MandiGlut() {
    std::cout << "[RUN] Testing rebindSlot (Dynamic Rerouting Recovery)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    auto params = createDefaultParams("CTR-REBIND-SLOT");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    assert(cmd.execute() == CommandStatus::Success);
    assert(slotPort.isSlotReserved(AgentId("Azadpur"), SimTime(480)));

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Rebind slot to Ghazipur
    AgentId newMandi("Ghazipur");
    SimTime newSlotTime(540);
    bool rebound = cmd.rebindSlot(newMandi, newSlotTime, SimTime(200));
    assert(rebound);
    assert(cmd.state() == ContractState::COMMITTED);
    assert(cmd.mandiId() == newMandi);
    assert(cmd.slotTime() == newSlotTime);

    // Old slot is released, new slot is reserved
    assert(!slotPort.isSlotReserved(AgentId("Azadpur"), SimTime(480)));
    assert(slotPort.isSlotReserved(newMandi, newSlotTime));
    assert(slotPort.activeReservationCount() == 1);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Now compensate and verify the NEW slot gets released
    assert(cmd.compensate() == CommandStatus::Success);
    assert(!slotPort.isSlotReserved(newMandi, newSlotTime));
    assert(slotPort.activeReservationCount() == 0);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    std::cout << "  [PASS] rebindSlot cleanly swapped slots and updated undo log." << std::endl;
}

void testRebindTruck_PeerHandoff() {
    std::cout << "[RUN] Testing rebindTruck (Peer Handoff Recovery)..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    auto params = createDefaultParams("CTR-REBIND-TRUCK");
    TransitContractCommand cmd(params, truckPort, slotPort, escrowPort);

    assert(cmd.execute() == CommandStatus::Success);
    assert(truckPort.isTruckLocked(AgentId("Truck-DL01")));

    std::vector<const TransitContractCommand*> contracts = {&cmd};
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Rebind truck to peer Truck-DL02
    AgentId peerTruck("Truck-DL02");
    bool rebound = cmd.rebindTruck(peerTruck, SimTime(220));
    assert(rebound);
    assert(cmd.state() == ContractState::COMMITTED);
    assert(cmd.transporterId() == peerTruck);

    // Old truck released, new truck locked
    assert(!truckPort.isTruckLocked(AgentId("Truck-DL01")));
    assert(truckPort.isTruckLocked(peerTruck));
    assert(truckPort.activeLockCount() == 1);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Complete contract on delivery by peer truck
    assert(cmd.complete(SimTime(650)));
    assert(!truckPort.isTruckLocked(peerTruck));
    assert(truckPort.activeLockCount() == 0);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    std::cout << "  [PASS] rebindTruck transferred manifest to peer with zero leaks." << std::endl;
}

void testMultipleConcurrentContractsAndInvariant() {
    std::cout << "[RUN] Testing Multiple Concurrent Contracts and Invariant Checking..." << std::endl;
    FakeTruckLockPort truckPort;
    FakeSlotPort slotPort;
    FakeEscrowPort escrowPort;

    std::vector<std::unique_ptr<TransitContractCommand>> cmdList;
    std::vector<const TransitContractCommand*> contracts;

    for (int i = 1; i <= 5; ++i) {
        auto p = createDefaultParams("CTR-MULTI-0" + std::to_string(i));
        p.transporterId = AgentId("Truck-" + std::to_string(i));
        p.slotTime = SimTime(400 + i * 20);
        auto cmd = std::make_unique<TransitContractCommand>(p, truckPort, slotPort, escrowPort);
        contracts.push_back(cmd.get());
        cmdList.push_back(std::move(cmd));
    }

    // Check invariant initially
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Execute all 5
    for (auto& cmd : cmdList) {
        assert(cmd->execute() == CommandStatus::Success);
    }
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);
    assert(truckPort.activeLockCount() == 5);
    assert(slotPort.activeReservationCount() == 5);
    assert(escrowPort.activeEscrowCount() == 5);

    // Contract 1 completes
    assert(cmdList[0]->complete(SimTime(600)));
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Contract 2 compensates (e.g. distress sale)
    assert(cmdList[1]->compensate() == CommandStatus::Success);
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Contract 3 rebinds slot
    assert(cmdList[2]->rebindSlot(AgentId("Vashi"), SimTime(800), SimTime(500)));
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Contract 4 rebinds truck
    assert(cmdList[3]->rebindTruck(AgentId("Truck-Peer99"), SimTime(510)));
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);

    // Remaining contracts finish
    assert(cmdList[2]->complete(SimTime(900)));
    assert(cmdList[3]->complete(SimTime(900)));
    assert(cmdList[4]->compensate() == CommandStatus::Success);

    // Final global invariant check: all 5 contracts terminal, 0 resources in ports
    assert(InvariantChecker::verify(contracts, truckPort, slotPort, escrowPort).passed);
    assert(truckPort.activeLockCount() == 0);
    assert(slotPort.activeReservationCount() == 0);
    assert(escrowPort.activeEscrowCount() == 0);

    std::cout << "  [PASS] Invariant verified continuously across multiple concurrent contracts." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "KisanSetu (MandiFlow) - Phase B1 Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    testHappyPath2PC();
    testFailureAtPrepareStep1_TruckLock();
    testFailureAtPrepareStep2_SlotReservation();
    testFailureAtPrepareStep3_EscrowHold();
    testIdempotentCompensation();
    testRebindSlot_MandiGlut();
    testRebindTruck_PeerHandoff();
    testMultipleConcurrentContractsAndInvariant();

    std::cout << "========================================" << std::endl;
    std::cout << "All Phase B1 Commitment & Rollback Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
