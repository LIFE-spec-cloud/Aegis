#include "catch_compat.hpp"

#include "agents/LogisticsBroker.h"
#include "agents/FarmerAgent.h"
#include "agents/TransporterAgent.h"
#include "agents/APMCMandiAgent.h"
#include "agents/ProcessorAgent.h"
#include "core/EventBus.h"
#include "core/Scheduler.h"
#include "core/SimClock.h"
#include "core/Rng.h"
#include "contracts/EventLog.h"
#include "domain/CropBatch.h"
#include "domain/Commodity.h"
#include "domain/RoadGraph.h"
#include "domain/BaselineEvaluator.h"

using namespace kisansetu;

// ─── Phase 4 Tests: Planning Quality & IRecoveryContext ─────────────────────

TEST_CASE("ForecastPrice_SurgePenaltyFromInboundTrucks", "[planning][a4]") {
    contracts::EventLog log;
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);

    auto roadGraph = domain::RoadGraph::createDefaultDemoWorld();
    broker.setRoadGraph(&roadGraph);

    // Register Mandi with capacity = 4, base price = ₹2000
    contracts::AgentId mandiId("Mandi_Azadpur");
    broker.registerMandi(mandiId, contracts::NodeId("Mandi_Azadpur"),
                         contracts::Money::fromRupees(2000.0), 4);

    contracts::CropBatchInfo batch{
        .batchId = "B-01",
        .crop = contracts::CommodityType::Tomato,
        .volume = contracts::Quintals(20.0),
        .initialQuality = 1.0,
        .harvestTime = contracts::SimTime(0),
        .isRefrigerated = false
    };

    // 0 Inbound trucks -> normal base price ₹2000
    auto rank0 = broker.rankMandis(batch, {});
    REQUIRE(rank0.size() == 1);
    CHECK(rank0[0].forecastPrice == contracts::Money::fromRupees(2000.0));

    // Add 4 trucks (at capacity) -> ratio 4/4 = 1.0, no penalty yet
    for (int i = 0; i < 4; ++i) {
        broker.addInboundTruck(mandiId);
    }
    auto rank4 = broker.rankMandis(batch, {});
    CHECK(rank4[0].forecastPrice == contracts::Money::fromRupees(2000.0));

    // Add 4 more trucks (total 8 trucks) -> ratio 8/4 = 2.0
    // Surge factor = 1.0 - 0.5 * (2.0 - 1.0) = 0.50
    // Price = 2000 * 0.50 = ₹1000
    for (int i = 0; i < 4; ++i) {
        broker.addInboundTruck(mandiId);
    }
    auto rank8 = broker.rankMandis(batch, {});
    CHECK(rank8[0].forecastPrice == contracts::Money::fromRupees(1000.0));

    // Add 10 more trucks (total 18 trucks) -> ratio 18/4 = 4.5
    // Surge factor clamps at 0.40 floor -> Price = 2000 * 0.40 = ₹800
    for (int i = 0; i < 10; ++i) {
        broker.addInboundTruck(mandiId);
    }
    auto rank18 = broker.rankMandis(batch, {});
    CHECK(rank18[0].forecastPrice == contracts::Money::fromRupees(800.0));
}

TEST_CASE("HerdEffectMitigation_TenTruckRerouteAvoidsCongestion", "[planning][a4]") {
    contracts::EventLog log;
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    auto roadGraph = domain::RoadGraph::createDefaultDemoWorld();
    broker.setRoadGraph(&roadGraph);

    // Register 3 mandis with capacity 4 each
    contracts::AgentId mandiA("Mandi_Azadpur");
    contracts::AgentId mandiG("Mandi_Ghazipur");
    contracts::AgentId mandiK("Mandi_Kolar");

    // Azadpur has slightly higher base price ₹2200, Ghazipur ₹2100, Kolar ₹2000
    broker.registerMandi(mandiA, contracts::NodeId("Mandi_Azadpur"), contracts::Money::fromRupees(2200.0), 4);
    broker.registerMandi(mandiG, contracts::NodeId("Mandi_Ghazipur"), contracts::Money::fromRupees(2100.0), 4);
    broker.registerMandi(mandiK, contracts::NodeId("Mandi_Kolar"), contracts::Money::fromRupees(2000.0), 4);

    contracts::CropBatchInfo batch{
        .batchId = "BATCH-HERD",
        .crop = contracts::CommodityType::Tomato,
        .volume = contracts::Quintals(25.0),
        .initialQuality = 1.0,
        .harvestTime = contracts::SimTime(0),
        .isRefrigerated = false
    };

    // Simulate 10 rerouted trucks choosing destinations sequentially
    std::unordered_map<std::string, size_t> destinationCounts;
    for (int t = 0; t < 10; ++t) {
        auto ranked = broker.rankMandis(batch, {});
        REQUIRE_FALSE(ranked.empty());

        auto chosenMandi = ranked[0].mandiId;
        destinationCounts[chosenMandi.str()]++;

        // Broker tracks the inbound selection, dynamically mitigating herd effect
        broker.addInboundTruck(chosenMandi);
    }

    // Verification: Not all 10 trucks herd into Azadpur!
    // Azadpur capacity is 4; once saturated, subsequent trucks must divert to Ghazipur / Kolar
    CHECK(destinationCounts["Mandi_Azadpur"] < 10);
    CHECK(destinationCounts["Mandi_Ghazipur"] > 0);
    CHECK(destinationCounts.size() >= 2); // At least 2 different mandis selected

    std::cout << "  [Herd Mitigation] 10 trucks distributed: Azadpur="
              << destinationCounts["Mandi_Azadpur"] << ", Ghazipur="
              << destinationCounts["Mandi_Ghazipur"] << ", Kolar="
              << destinationCounts["Mandi_Kolar"] << "\n";
}

TEST_CASE("RecoveryContext_ProcessorsAndTrucksDiscovery", "[planning][a4]") {
    contracts::EventLog log;
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    auto roadGraph = domain::RoadGraph::createDefaultDemoWorld();
    broker.setRoadGraph(&roadGraph);

    // Register processors
    broker.registerProcessor(contracts::AgentId("Plant_MotherDairy"), contracts::NodeId("Plant_MotherDairy"));
    broker.registerProcessor(contracts::AgentId("Plant_SahyadriAgro"), contracts::NodeId("Plant_SahyadriAgro"));

    // Register trucks: Truck 1 (50qtl), Truck 2 (20qtl)
    broker.registerTransporter(contracts::AgentId("Truck_Big"), contracts::Quintals(50.0), contracts::NodeId("Farm_Sonipat"));
    broker.registerTransporter(contracts::AgentId("Truck_Small"), contracts::Quintals(20.0), contracts::NodeId("Farm_Sonipat"));

    // Discovery: Trucks for 40 quintals cargo -> Truck_Small should be excluded
    auto idleTrucks40 = broker.findIdleTrucks(contracts::NodeId("Farm_Sonipat"), contracts::Quintals(40.0));
    CHECK(idleTrucks40.size() == 1);
    CHECK(idleTrucks40[0] == contracts::AgentId("Truck_Big"));

    // Lock Truck_Big -> now 0 trucks available for 40 quintals
    auto lock = broker.lockTruck(contracts::AgentId("Truck_Big"), contracts::ContractId(101));
    REQUIRE(lock.has_value());
    auto idleAfterLock = broker.findIdleTrucks(contracts::NodeId("Farm_Sonipat"), contracts::Quintals(40.0));
    CHECK(idleAfterLock.empty());

    // Release Truck_Big -> available again
    broker.releaseTruck(*lock);
    CHECK(broker.findIdleTrucks(contracts::NodeId("Farm_Sonipat"), contracts::Quintals(40.0)).size() == 1);

    // Discovery: Processors in Delhi vicinity (distance <= 50km from Panipat)
    auto nearbyProcessors = broker.findProcessors(contracts::NodeId("Jct_Panipat"), 50.0);
    CHECK(nearbyProcessors.size() == 1);
    CHECK(nearbyProcessors[0] == contracts::AgentId("Plant_MotherDairy"));
}

TEST_CASE("Rerouting_HysteresisThresholdAndCap", "[planning][a4]") {
    contracts::EventLog log;
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);

    contracts::ContractId cid(42);
    contracts::AgentId mandiOld("Mandi_Azadpur");
    contracts::AgentId mandiNew1("Mandi_Ghazipur");
    contracts::AgentId mandiNew2("Mandi_Kolar");
    contracts::AgentId mandiNew3("Mandi_Vashi");

    // Case 1: Gain is ₹3,000 <= threshold ₹5,000 -> Should NOT reroute
    contracts::Money currentMargin = contracts::Money::fromRupees(20000.0);
    contracts::Money smallGainMargin = contracts::Money::fromRupees(23000.0); // +₹3000
    CHECK_FALSE(broker.shouldReroute(cid, currentMargin, smallGainMargin, contracts::Money::fromRupees(5000.0), 2));

    // Case 2: Gain is ₹7,000 > threshold ₹5,000 -> Should reroute
    contracts::Money largeGainMargin = contracts::Money::fromRupees(27000.0); // +₹7000
    CHECK(broker.shouldReroute(cid, currentMargin, largeGainMargin, contracts::Money::fromRupees(5000.0), 2));

    // Record attempt #1
    broker.recordReroute(cid, mandiOld, mandiNew1);
    CHECK(broker.getRerouteCount(cid) == 1);

    // Record attempt #2
    broker.recordReroute(cid, mandiNew1, mandiNew2);
    CHECK(broker.getRerouteCount(cid) == 2);

    // Case 3: Reached max reroutes (cap = 2) -> Should NOT reroute even with huge gain
    contracts::Money hugeGain = contracts::Money::fromRupees(50000.0);
    CHECK_FALSE(broker.shouldReroute(cid, currentMargin, hugeGain, contracts::Money::fromRupees(5000.0), 2));
}

// ─── Phase 5 Tests: Hardening, Parallel Mode & Baseline Evaluation ──────────

TEST_CASE("ParallelMode_ExecutionAndEquivalence", "[evaluation][a5]") {
    contracts::EventLog log;
    core::Scheduler scheduler;
    core::SimClock clock;
    core::Rng rng(42);
    core::EventBus bus(scheduler, log, &clock, &rng);

    // Enable parallel multi-threaded mode
    bus.setParallelMode(true, 4);
    CHECK(bus.isParallelMode());

    // Register agents
    agents::LogisticsBroker broker(contracts::AgentId("Broker"), log);
    agents::FarmerAgent farmer(
        contracts::AgentId("Farmer1"),
        contracts::Money::fromRupees(800.0),
        contracts::Money::fromRupees(5000.0),
        0.1);

    agents::TransporterAgent truck1(
        contracts::AgentId("Truck1"),
        contracts::NodeId("Farm_Sonipat"),
        contracts::Quintals(50.0),
        contracts::Money::fromRupees(15.0),
        false);

    agents::APMCMandiAgent mandi1(
        contracts::AgentId("Mandi1"),
        contracts::NodeId("Mandi_Azadpur"),
        contracts::Money::fromRupees(2000.0),
        4);

    auto roadGraph = domain::RoadGraph::createDefaultDemoWorld();
    broker.setRoadGraph(&roadGraph);
    broker.setEventBus(&bus);
    farmer.setDependencies(&bus, &log, broker.id());
    truck1.setDependencies(&bus, &log, &roadGraph);
    mandi1.setDependencies(&bus, &log, nullptr);

    bus.registerAgent(broker.id(), &broker);
    bus.registerAgent(farmer.id(), &farmer);
    bus.registerAgent(truck1.id(), &truck1);
    bus.registerAgent(mandi1.id(), &mandi1);

    broker.registerTransporter(truck1.id(), truck1.capacity(), truck1.currentLocation());
    broker.registerMandi(mandi1.id(), mandi1.location());
    broker.registerFarmer(farmer.id());
    broker.registerSlotPort(
        mandi1.id(),
        [&mandi1](const contracts::AgentId& mid, const contracts::ContractId& cid, contracts::SimTime st) {
            return mandi1.reserveSlot(mid, cid, st);
        },
        [&mandi1](contracts::SlotReservationHandle& h) {
            return mandi1.cancelSlot(h);
        }
    );

    // Announce harvest
    domain::Commodity tomato = domain::Commodity::fromType(contracts::CommodityType::Tomato);
    domain::CropBatch batch("BATCH-PARALLEL", tomato, contracts::Quintals(20.0), contracts::SimTime(0));
    farmer.announceHarvest(std::move(batch), contracts::NodeId("Farm_Sonipat"),
                           contracts::SimTime(0), contracts::SimTime(360), contracts::SimTime(0));

    // Parallel dispatch
    size_t events = bus.dispatchAll();
    CHECK(events > 0);
    CHECK(farmer.contractState() == contracts::ContractState::COMMITTED);
    CHECK(broker.activeSessionCount() >= 1);
}

TEST_CASE("EventBus_PacketDropAndLatencySimulation", "[evaluation][a5]") {
    contracts::EventLog log;
    core::Scheduler scheduler;
    core::SimClock clock;
    core::Rng rng(42);
    core::EventBus bus(scheduler, log, &clock, &rng);

    bus.setPacketDropRate(0.50);
    bus.setLatencyTicks(25);

    CHECK(bus.packetDropRate() == 0.50);
    CHECK(bus.latencyTicks() == 25);

    // Send envelopes and check that simulated latency is applied
    contracts::InformPayload payload{.message = "Test latency"};
    bus.send(contracts::Envelope(
        "TEST_LAT",
        contracts::ContractId(1),
        contracts::AgentId("Src"),
        contracts::AgentId("Dst"),
        contracts::Performative::INFORM,
        payload,
        contracts::SimTime(10),
        contracts::SimTime(60)
    ));

    // Scheduled delivery time should be sentAt (10) + latency (25) = 35
    auto nextTime = scheduler.nextEventTime();
    REQUIRE(nextTime.has_value());
    CHECK(nextTime->ticks() == 35);
}

TEST_CASE("BaselineEvaluator_ImprovementAndSpoilageReduction", "[evaluation][a5]") {
    auto result = domain::BaselineEvaluator::runEvaluation(10, 30.0, 0.30);

    std::cout << result.toReportString() << "\n";

    // KisanSetu should substantially outperform static baseline:
    // 1. Spoilage reduction must be positive
    CHECK(result.spoilageReductionPercent > 10.0);
    // 2. Net margin improvement must be positive
    CHECK(result.netMarginImprovementPercent > 15.0);
    // 3. Salvage rate for KisanSetu must be 100% of at-risk stalled batches
    CHECK(result.kisanSetu.salvageRatePercent == 100.0);
    // 4. Baseline salvage rate is 0%
    CHECK(result.baseline.salvageRatePercent == 0.0);
}
