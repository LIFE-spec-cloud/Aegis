#include "catch_compat.hpp"
#include "contracts/Types.h"
#include "contracts/Payloads.h"
#include "contracts/Envelope.h"
#include "contracts/IReservationPorts.h"
#include "contracts/ITelemetry.h"
#include "contracts/IRecoveryContext.h"
#include "contracts/IOrderInputPort.h"
#include "contracts/EventLog.h"

#include <type_traits>
#include <string>

using namespace kisansetu::contracts;

TEST_CASE("StrongTypes_BasicSemantics", "[contracts]") {
    // AgentId
    AgentId a1("Farmer_1");
    AgentId a2("Farmer_1");
    AgentId a3("Transporter_1");
    CHECK(a1 == a2);
    CHECK(a1 != a3);
    CHECK(a1.str() == "Farmer_1");
    CHECK(!a1.empty());

    // ContractId
    ContractId c1(101);
    ContractId c2(101);
    ContractId c3(102);
    CHECK(c1 == c2);
    CHECK(c1 != c3);
    CHECK(c1.value() == 101);
    CHECK(c1.str() == "CTR-101");

    // Money (integer paise)
    Money m1 = Money::fromRupees(2200.0);
    CHECK(m1.toPaise() == 220000);
    CHECK(m1.toRupees() == 2200.0);

    Money m2 = Money::fromPaise(5000);
    CHECK(m2.toRupees() == 50.0);

    Money sum = m1 + m2;
    CHECK(sum.toPaise() == 225000);

    Money diff = m1 - m2;
    CHECK(diff.toPaise() == 215000);

    Money scaled = m2 * 1.5;
    CHECK(scaled.toPaise() == 7500);

    // Quintals
    Quintals q1(50.0);
    CHECK(q1.value() == 50.0);
    CHECK(q1.toKg() == 5000.0);

    Quintals q2 = Quintals::fromKg(250.0);
    CHECK(q2.value() == 2.5);

    // SimTime and Hours
    SimTime t1(420); // 7 hours
    CHECK(t1.ticks() == 420);
    CHECK(t1.toHours() == 7.0);

    Hours h1(7.0);
    CHECK(h1.toSimTime() == t1);
    CHECK(Hours(t1).value() == 7.0);

    // Enums
    CHECK(toString(CommodityType::Tomato) == "Tomato");
    CHECK(toString(FreshnessGrade::GradeA) == "GradeA");
    CHECK(toString(TruckState::EN_ROUTE) == "EN_ROUTE");
    CHECK(toString(ContractState::COMMITTED) == "COMMITTED");
    CHECK(toString(Performative::TASK_ANNOUNCEMENT) == "TASK_ANNOUNCEMENT");
}

TEST_CASE("EnvelopeAndPayload_SumTypeHandling", "[contracts]") {
    TaskAnnouncementPayload task{
        .farmerId = AgentId("Farmer_1"),
        .crop = CommodityType::Tomato,
        .volume = Quintals(50.0),
        .readyTime = SimTime(0),
        .deadline = SimTime(600),
        .reservePricePerQuintal = Money::fromRupees(1800.0),
        .farmLocation = NodeId("Farm_Sonipat")
    };

    Envelope env(
        "MSG-001",
        ContractId(1),
        AgentId("Farmer_1"),
        AgentId("Broker"),
        Performative::TASK_ANNOUNCEMENT,
        task,
        SimTime(0),
        SimTime(60)
    );

    CHECK(env.messageId() == "MSG-001");
    CHECK(env.correlationId().value() == 1);
    CHECK(env.sender().str() == "Farmer_1");
    CHECK(env.receiver().str() == "Broker");
    CHECK(env.performative() == Performative::TASK_ANNOUNCEMENT);
    CHECK(!env.isExpired(SimTime(30)));
    CHECK(env.isExpired(SimTime(70)));

    CHECK(std::holds_alternative<TaskAnnouncementPayload>(env.payload()));
    const auto& extracted = std::get<TaskAnnouncementPayload>(env.payload());
    CHECK(extracted.crop == CommodityType::Tomato);
    CHECK(extracted.volume.value() == 50.0);
}

TEST_CASE("ReservationHandles_MoveOnlySemantics", "[contracts]") {
    // Verify move-only at compile time
    STATIC_REQUIRE(!std::is_copy_constructible_v<SlotReservationHandle>);
    STATIC_REQUIRE(!std::is_copy_assignable_v<SlotReservationHandle>);
    STATIC_REQUIRE(std::is_move_constructible_v<SlotReservationHandle>);
    STATIC_REQUIRE(std::is_move_assignable_v<SlotReservationHandle>);

    STATIC_REQUIRE(!std::is_copy_constructible_v<TruckLockHandle>);
    STATIC_REQUIRE(!std::is_copy_assignable_v<TruckLockHandle>);
    STATIC_REQUIRE(std::is_move_constructible_v<TruckLockHandle>);
    STATIC_REQUIRE(std::is_move_assignable_v<TruckLockHandle>);

    STATIC_REQUIRE(!std::is_copy_constructible_v<EscrowHandle>);
    STATIC_REQUIRE(!std::is_copy_assignable_v<EscrowHandle>);
    STATIC_REQUIRE(std::is_move_constructible_v<EscrowHandle>);
    STATIC_REQUIRE(std::is_move_assignable_v<EscrowHandle>);

    // SlotReservationHandle
    SlotReservationHandle slot(AgentId("Mandi_Azadpur"), ContractId(101), SimTime(300), 1);
    CHECK(slot.isValid());
    CHECK(slot.mandiId().str() == "Mandi_Azadpur");

    SlotReservationHandle movedSlot = std::move(slot);
    CHECK(!slot.isValid());
    CHECK(movedSlot.isValid());
    CHECK(movedSlot.contractId().value() == 101);

    // TruckLockHandle and rebind
    TruckLockHandle truck(AgentId("Truck_1"), ContractId(101), 1);
    CHECK(truck.isValid());
    CHECK(truck.truckId().str() == "Truck_1");

    truck.rebind(AgentId("Truck_2"), 2);
    CHECK(truck.isValid());
    CHECK(truck.truckId().str() == "Truck_2");
    CHECK(truck.lockId() == 2);

    // EscrowHandle
    EscrowHandle escrow(AgentId("Farmer_1"), ContractId(101), Money::fromRupees(500.0), 1);
    CHECK(escrow.isValid());
    CHECK(escrow.amount().toRupees() == 500.0);
}

TEST_CASE("TelemetryAndLog_FormattingAndFiltering", "[contracts]") {
    TelemetryEvent te{
        .timestamp = SimTime(120),
        .truckId = AgentId("Truck_1"),
        .position = NodeId("Jct_Panipat"),
        .speedKmh = 45.0,
        .state = TruckState::EN_ROUTE,
        .predictedArrival = SimTime(180),
        .currentQuality = 0.88,
        .batchId = "BATCH-T01"
    };

    std::string teStr = te.toLogString();
    CHECK(teStr.find("Truck=Truck_1") != std::string::npos);
    CHECK(teStr.find("Q=0.880") != std::string::npos);

    // EventLog
    EventLog log;
    log.log(SimTime(10), "TASK_ANNOUNCED", AgentId("Farmer_1"), AgentId("Broker"), ContractId(1), "Tomato batch announced");
    log.log(SimTime(20), "PROPOSAL_SUBMITTED", AgentId("Truck_1"), AgentId("Broker"), ContractId(1), "Bid freight ₹1500");
    log.log(SimTime(30), "TASK_ANNOUNCED", AgentId("Farmer_2"), AgentId("Broker"), ContractId(2), "Onion batch announced");

    CHECK(log.size() == 3);

    auto forC1 = log.filterByContract(ContractId(1));
    CHECK(forC1.size() == 2);

    auto announced = log.filterByType("TASK_ANNOUNCED");
    CHECK(announced.size() == 2);
}
