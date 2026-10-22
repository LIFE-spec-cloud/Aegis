#include "catch_compat.hpp"

#include "agents/LogisticsBroker.h"
#include "agents/FarmerAgent.h"
#include "agents/APMCMandiAgent.h"
#include "agents/TransporterAgent.h"
#include "agents/ProcessorAgent.h"
#include "core/EventBus.h"
#include "core/Scheduler.h"
#include "core/SimClock.h"
#include "core/Rng.h"
#include "contracts/EventLog.h"
#include "domain/CropBatch.h"
#include "domain/Commodity.h"
#include "domain/RoadGraph.h"

using namespace kisansetu;

// ─── Test: CNP Happy Path ───────────────────────────────────────────────────

TEST_CASE("CNP_HappyPath_FullNegotiationToCommit", "[cnp][integration]") {
    contracts::EventLog log;
    core::Scheduler scheduler;
    core::SimClock clock;
    core::Rng rng(42);
    core::EventBus bus(scheduler, log, &clock, &rng);

    // Create agents
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    agents::FarmerAgent farmer(
        contracts::AgentId("Farmer1"),
        contracts::Money::fromRupees(800.0),   // reserve price
        contracts::Money::fromRupees(5000.0),  // deposit budget
        0.1);                                   // risk aversion

    agents::TransporterAgent truck1(
        contracts::AgentId("Truck1"),
        contracts::NodeId("Depot_A"),
        contracts::Quintals(50.0),
        contracts::Money::fromRupees(15.0),  // cost per km
        false);

    agents::APMCMandiAgent mandi1(
        contracts::AgentId("Mandi1"),
        contracts::NodeId("Azadpur_APMC"),
        contracts::Money::fromRupees(2000.0),  // base price per quintal
        4);                                     // gate quota per hour

    auto roadGraph = domain::RoadGraph::createDefaultDemoWorld();

    // Set dependencies
    broker.setEventBus(&bus);
    farmer.setDependencies(&bus, &log, contracts::AgentId("Broker"));
    truck1.setDependencies(&bus, &log, &roadGraph);
    mandi1.setDependencies(&bus, &log, nullptr);

    // Register agents with EventBus
    bus.registerAgent(broker.id(), &broker);
    bus.registerAgent(farmer.id(), &farmer);
    bus.registerAgent(truck1.id(), &truck1);
    bus.registerAgent(mandi1.id(), &mandi1);

    // Register agents with Broker
    broker.registerTransporter(truck1.id());
    broker.registerMandi(mandi1.id(), mandi1.location());
    broker.registerFarmer(farmer.id());

    // Register Mandi's slot port with Broker
    broker.registerSlotPort(
        mandi1.id(),
        [&mandi1](const contracts::AgentId& mid, const contracts::ContractId& cid, contracts::SimTime st) {
            return mandi1.reserveSlot(mid, cid, st);
        },
        [&mandi1](contracts::SlotReservationHandle& h) {
            return mandi1.cancelSlot(h);
        }
    );

    // Farmer announces harvest
    domain::Commodity tomato = domain::Commodity::fromType(contracts::CommodityType::Tomato);
    domain::CropBatch batch("BATCH-001", tomato, contracts::Quintals(20.0), contracts::SimTime(0));
    farmer.announceHarvest(std::move(batch), contracts::NodeId("Farm_Farmer1"),
                           contracts::SimTime(0), contracts::SimTime(360), contracts::SimTime(0));

    // Dispatch all events through the bus to let the CNP run
    size_t dispatched = bus.dispatchAll();

    // Verification: A session was created in the broker
    CHECK(broker.activeSessionCount() >= 1);

    // Verification: The farmer should have received proposals and either committed or moved to preparing
    contracts::ContractState farmerState = farmer.contractState();
    // The farmer should have progressed past CREATED
    CHECK(farmerState != contracts::ContractState::CREATED);

    // Verification: Events were logged
    CHECK(log.size() > 5);

    // Verification: The session in broker should have received proposals
    auto session = broker.findSession(contracts::ContractId(1));
    if (session != nullptr) {
        CHECK(session->transporterProposals.size() >= 1);
        CHECK(session->mandiProposals.size() >= 1);
    }

    std::cout << "  [CNP HappyPath] Total dispatched events: " << dispatched << "\n";
    std::cout << "  [CNP HappyPath] Event log entries: " << log.size() << "\n";
    std::cout << "  [CNP HappyPath] Farmer contract state: " << contracts::toString(farmerState) << "\n";
}

// ─── Test: Last Slot Race ───────────────────────────────────────────────────

TEST_CASE("CNP_LastSlotRace_DeterministicOutcome", "[cnp][integration]") {
    contracts::EventLog log;
    core::Scheduler scheduler;
    core::SimClock clock;
    core::Rng rng(42);
    core::EventBus bus(scheduler, log, &clock, &rng);

    // Create agents
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    broker.setEventBus(&bus);

    // Create a mandi with ONLY 1 gate slot per hour (quota = 1)
    agents::APMCMandiAgent mandi(
        contracts::AgentId("Mandi_Tight"),
        contracts::NodeId("Tight_APMC"),
        contracts::Money::fromRupees(1800.0),
        1);  // Only 1 slot per hour!
    mandi.setDependencies(&bus, &log, nullptr);

    bus.registerAgent(broker.id(), &broker);
    bus.registerAgent(mandi.id(), &mandi);

    broker.registerMandi(mandi.id(), mandi.location());
    broker.registerSlotPort(
        mandi.id(),
        [&mandi](const contracts::AgentId& mid, const contracts::ContractId& cid, contracts::SimTime st) {
            return mandi.reserveSlot(mid, cid, st);
        },
        [&mandi](contracts::SlotReservationHandle& h) {
            return mandi.cancelSlot(h);
        }
    );

    // Simulate two farmers competing for the same slot
    contracts::SimTime slotTime(120); // Hour 2

    // First farmer gets the slot
    auto slot1 = mandi.reserveSlot(mandi.id(), contracts::ContractId(100), slotTime);
    CHECK(slot1.has_value());

    // Second farmer tries — should FAIL (quota full)
    auto slot2 = mandi.reserveSlot(mandi.id(), contracts::ContractId(200), slotTime);
    CHECK_FALSE(slot2.has_value());

    // Occupancy should be exactly 1
    CHECK(mandi.getOccupancy(slotTime) == 1);

    // Cancel first slot — opens up
    if (slot1.has_value()) {
        CHECK(mandi.cancelSlot(*slot1));
    }

    // Now second farmer can get the slot
    auto slot3 = mandi.reserveSlot(mandi.id(), contracts::ContractId(200), slotTime);
    CHECK(slot3.has_value());

    // Clean up
    if (slot3.has_value()) {
        mandi.cancelSlot(*slot3);
    }

    std::cout << "  [LastSlotRace] Slot contention verified deterministically\n";
    std::cout << "  [LastSlotRace] Event log entries: " << log.size() << "\n";
}

// ─── Test: Broker Two-Phase Commit Rollback ─────────────────────────────────

TEST_CASE("CNP_CommitRollback_OnSlotFailure", "[cnp][integration]") {
    contracts::EventLog log;
    core::Scheduler scheduler;
    core::SimClock clock;
    core::EventBus bus(scheduler, log, &clock);

    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    broker.setEventBus(&bus);

    // Create mandi with quota 1
    agents::APMCMandiAgent mandi(
        contracts::AgentId("Mandi_Full"),
        contracts::NodeId("Full_APMC"),
        contracts::Money::fromRupees(1500.0),
        1);
    mandi.setDependencies(&bus, &log, nullptr);

    bus.registerAgent(broker.id(), &broker);
    bus.registerAgent(mandi.id(), &mandi);

    broker.registerMandi(mandi.id(), mandi.location());
    broker.registerTransporter(contracts::AgentId("TestTruck"));

    broker.registerSlotPort(
        mandi.id(),
        [&mandi](const contracts::AgentId& mid, const contracts::ContractId& cid, contracts::SimTime st) {
            return mandi.reserveSlot(mid, cid, st);
        },
        [&mandi](contracts::SlotReservationHandle& h) {
            return mandi.cancelSlot(h);
        }
    );

    // Pre-fill the only slot
    auto preFilledSlot = mandi.reserveSlot(mandi.id(), contracts::ContractId(999), contracts::SimTime(120));
    CHECK(preFilledSlot.has_value());

    // Now try to commit a contract that needs a slot at the same hour
    contracts::AcceptPayload accept{
        .contractId = contracts::ContractId(50),
        .farmerId = contracts::AgentId("Farmer_X"),
        .transporterId = contracts::AgentId("TestTruck"),
        .mandiId = mandi.id(),
        .agreedFreight = contracts::Money::fromRupees(3000.0),
        .expectedMandiPrice = contracts::Money::fromRupees(2000.0),
        .scheduledGateSlot = contracts::SimTime(120),
        .expectedArrival = contracts::SimTime(115)
    };

    // Manually create a session for the test
    agents::ContractSession session;
    session.contractId = contracts::ContractId(50);
    session.farmerId = contracts::AgentId("Farmer_X");
    session.state = contracts::ContractState::PREPARING;

    // Deliver farmer accept to broker
    contracts::Envelope acceptEnv(
        "TEST_ACCEPT", contracts::ContractId(50),
        contracts::AgentId("Farmer_X"), broker.id(),
        contracts::Performative::ACCEPT, accept,
        contracts::SimTime(10), contracts::SimTime(60));

    // First, we need a session in the broker: simulate task announcement
    contracts::TaskAnnouncementPayload ann{
        .farmerId = contracts::AgentId("Farmer_X"),
        .crop = contracts::CommodityType::Tomato,
        .volume = contracts::Quintals(10.0),
        .readyTime = contracts::SimTime(0),
        .deadline = contracts::SimTime(360),
        .reservePricePerQuintal = contracts::Money::fromRupees(800.0),
        .farmLocation = contracts::NodeId("Farm_X")
    };
    contracts::Envelope annEnv(
        "TEST_ANN", contracts::ContractId(0),
        contracts::AgentId("Farmer_X"), broker.id(),
        contracts::Performative::TASK_ANNOUNCEMENT, ann,
        contracts::SimTime(0), contracts::SimTime(60));
    broker.onMessage(annEnv);

    // Now the broker has a session with ContractId(1)
    auto* sess = broker.findSession(contracts::ContractId(1));
    CHECK(sess != nullptr);

    // Attempt to accept with the correct contract ID
    contracts::AcceptPayload accept2{
        .contractId = contracts::ContractId(1),
        .farmerId = contracts::AgentId("Farmer_X"),
        .transporterId = contracts::AgentId("TestTruck"),
        .mandiId = mandi.id(),
        .agreedFreight = contracts::Money::fromRupees(3000.0),
        .expectedMandiPrice = contracts::Money::fromRupees(2000.0),
        .scheduledGateSlot = contracts::SimTime(120), // Same hour as the pre-filled slot!
        .expectedArrival = contracts::SimTime(115)
    };

    contracts::Envelope acceptEnv2(
        "TEST_ACCEPT2", contracts::ContractId(1),
        contracts::AgentId("Farmer_X"), broker.id(),
        contracts::Performative::ACCEPT, accept2,
        contracts::SimTime(10), contracts::SimTime(60));
    broker.onMessage(acceptEnv2);

    // The commit should have FAILED because the slot was already taken
    sess = broker.findSession(contracts::ContractId(1));
    CHECK(sess != nullptr);
    CHECK(sess->state == contracts::ContractState::ABORTED);

    // Truck should have been released after rollback
    auto idleTrucks = broker.findIdleTrucks(contracts::NodeId(""), contracts::Quintals(0.0));
    // TestTruck was registered but should have been unlocked after rollback
    bool truckIdle = false;
    for (const auto& tid : idleTrucks) {
        if (tid == contracts::AgentId("TestTruck")) {
            truckIdle = true;
        }
    }
    CHECK(truckIdle);

    // Clean up
    if (preFilledSlot.has_value()) {
        mandi.cancelSlot(*preFilledSlot);
    }

    std::cout << "  [CommitRollback] Two-phase commit rollback verified\n";
    std::cout << "  [CommitRollback] Event log entries: " << log.size() << "\n";
}

// ─── Test: Broker Failure Handling ──────────────────────────────────────────

TEST_CASE("CNP_FailureHandling_TruckBreakdown", "[cnp][integration]") {
    contracts::EventLog log;
    core::Scheduler scheduler;
    core::SimClock clock;
    core::EventBus bus(scheduler, log, &clock);

    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    broker.setEventBus(&bus);
    bus.registerAgent(broker.id(), &broker);

    // Create a session manually by sending a task announcement
    contracts::TaskAnnouncementPayload ann{
        .farmerId = contracts::AgentId("Farmer_F"),
        .crop = contracts::CommodityType::Onion,
        .volume = contracts::Quintals(30.0),
        .readyTime = contracts::SimTime(0),
        .deadline = contracts::SimTime(480),
        .reservePricePerQuintal = contracts::Money::fromRupees(700.0),
        .farmLocation = contracts::NodeId("Farm_F")
    };
    contracts::Envelope annEnv(
        "ANN_F", contracts::ContractId(0),
        contracts::AgentId("Farmer_F"), broker.id(),
        contracts::Performative::TASK_ANNOUNCEMENT, ann,
        contracts::SimTime(0), contracts::SimTime(60));
    broker.onMessage(annEnv);

    auto* sess = broker.findSession(contracts::ContractId(1));
    CHECK(sess != nullptr);
    CHECK(sess->state == contracts::ContractState::CREATED);

    // Simulate truck breakdown
    contracts::FailureNoticePayload failure{
        .affectedAgentId = contracts::AgentId("Truck_B"),
        .contractId = contracts::ContractId(1),
        .location = contracts::NodeId("Highway_NH48"),
        .failureType = "TRUCK_BREAKDOWN",
        .details = "Severe axle failure at km 120",
        .timestamp = contracts::SimTime(90)
    };
    contracts::Envelope failEnv(
        "FAIL_B", contracts::ContractId(1),
        contracts::AgentId("Truck_B"), broker.id(),
        contracts::Performative::FAILURE, failure,
        contracts::SimTime(90), contracts::SimTime(60));
    broker.onMessage(failEnv);

    // Session should transition to COMPENSATED
    sess = broker.findSession(contracts::ContractId(1));
    CHECK(sess != nullptr);
    CHECK(sess->state == contracts::ContractState::COMPENSATED);

    std::cout << "  [FailureHandling] Broker failure response verified\n";
}

// ─── Test: Agent Registration and Idle Truck Discovery ──────────────────────

TEST_CASE("BrokerRegistry_FindIdleTrucksExcludesLocked", "[cnp][broker]") {
    contracts::EventLog log;

    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    broker.registerTransporter(contracts::AgentId("Truck_A"));
    broker.registerTransporter(contracts::AgentId("Truck_B"));
    broker.registerTransporter(contracts::AgentId("Truck_C"));

    // All trucks idle initially
    auto idle = broker.findIdleTrucks(contracts::NodeId("any"), contracts::Quintals(0.0));
    CHECK(idle.size() == 3);

    // Lock one truck
    auto handle = broker.lockTruck(contracts::AgentId("Truck_B"), contracts::ContractId(42));
    CHECK(handle.has_value());

    // Now only 2 idle
    idle = broker.findIdleTrucks(contracts::NodeId("any"), contracts::Quintals(0.0));
    CHECK(idle.size() == 2);

    // Release and verify back to 3
    broker.releaseTruck(*handle);
    idle = broker.findIdleTrucks(contracts::NodeId("any"), contracts::Quintals(0.0));
    CHECK(idle.size() == 3);

    std::cout << "  [BrokerRegistry] Idle truck tracking verified\n";
}
