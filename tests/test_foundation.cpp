#include "catch_compat.hpp"
#include "core/Rng.h"
#include "core/SimClock.h"
#include "core/Scheduler.h"
#include "core/Agent.h"
#include "core/EventBus.h"
#include "contracts/EventLog.h"
#include "contracts/Payloads.h"

#include <vector>
#include <string>

using namespace kisansetu::core;
using namespace kisansetu::contracts;

namespace {

class TestEchoAgent : public Agent {
public:
    explicit TestEchoAgent(AgentId id) : Agent(std::move(id)) {}

    void onMessage(const Envelope& envelope) override {
        receivedMessages.push_back(envelope.messageId());
    }

    std::vector<std::string> receivedMessages;
};

} // namespace

TEST_CASE("SimClock_AdvanceAndReset", "[foundation]") {
    SimClock clock;
    CHECK(clock.now().ticks() == 0);

    clock.advanceBy(SimTime(60));
    CHECK(clock.now().ticks() == 60);
    CHECK(clock.now().toHours() == 1.0);

    clock.advanceTo(SimTime(300));
    CHECK(clock.now().ticks() == 300);
    CHECK(clock.now().toHours() == 5.0);

    clock.reset();
    CHECK(clock.now().ticks() == 0);
}

TEST_CASE("Rng_DeterminismAndDistributions", "[foundation]") {
    Rng rng1(42);
    Rng rng2(42);

    // Verify same seed produces identical sequence
    for (int i = 0; i < 50; ++i) {
        CHECK(rng1.nextU64() == rng2.nextU64());
        CHECK(rng1.uniformDouble(0.0, 100.0) == rng2.uniformDouble(0.0, 100.0));
        CHECK(rng1.uniformInt(1, 10) == rng2.uniformInt(1, 10));
        CHECK(rng1.bernoulli(0.3) == rng2.bernoulli(0.3));
    }

    // Different seed produces different values
    Rng rng3(999);
    CHECK(rng1.nextU64() != rng3.nextU64());
}

TEST_CASE("Scheduler_DeterministicEventOrdering", "[foundation]") {
    Scheduler scheduler;
    InformPayload payload{"ping"};

    // Schedule events out of chronological order
    scheduler.schedule(SimTime(100), Envelope("MSG_100_A", ContractId(1), AgentId("A"), AgentId("B"), Performative::INFORM, payload, SimTime(0), SimTime(500)));
    scheduler.schedule(SimTime(20),  Envelope("MSG_20",    ContractId(1), AgentId("A"), AgentId("B"), Performative::INFORM, payload, SimTime(0), SimTime(500)));
    scheduler.schedule(SimTime(100), Envelope("MSG_100_B", ContractId(1), AgentId("A"), AgentId("B"), Performative::INFORM, payload, SimTime(0), SimTime(500)));
    scheduler.schedule(SimTime(50),  Envelope("MSG_50",    ContractId(1), AgentId("A"), AgentId("B"), Performative::INFORM, payload, SimTime(0), SimTime(500)));
    scheduler.schedule(SimTime(10),  Envelope("MSG_10",    ContractId(1), AgentId("A"), AgentId("B"), Performative::INFORM, payload, SimTime(0), SimTime(500)));

    REQUIRE(scheduler.size() == 5);

    // Assert exact chronological order: 10, 20, 50, 100_A, 100_B
    auto e1 = scheduler.pop();
    CHECK(e1.time.ticks() == 10);
    CHECK(e1.envelope.messageId() == "MSG_10");

    auto e2 = scheduler.pop();
    CHECK(e2.time.ticks() == 20);
    CHECK(e2.envelope.messageId() == "MSG_20");

    auto e3 = scheduler.pop();
    CHECK(e3.time.ticks() == 50);
    CHECK(e3.envelope.messageId() == "MSG_50");

    // Tied at simTime 100: order must strictly follow insertion sequence number
    auto e4 = scheduler.pop();
    CHECK(e4.time.ticks() == 100);
    CHECK(e4.envelope.messageId() == "MSG_100_A");

    auto e5 = scheduler.pop();
    CHECK(e5.time.ticks() == 100);
    CHECK(e5.envelope.messageId() == "MSG_100_B");

    CHECK(scheduler.empty());
}

TEST_CASE("EventBus_AgentMailboxAndDispatching", "[foundation]") {
    Scheduler scheduler;
    EventLog eventLog;
    SimClock clock;
    EventBus bus(scheduler, eventLog, &clock);

    TestEchoAgent farmer(AgentId("Farmer_1"));
    TestEchoAgent mandi(AgentId("Mandi_1"));

    bus.registerAgent(farmer.id(), &farmer);
    bus.registerAgent(mandi.id(), &mandi);

    CHECK(bus.registeredAgentCount() == 2);
    CHECK(bus.hasAgent(AgentId("Farmer_1")));

    InformPayload payload{"order"};
    bus.sendAt(Envelope("MSG-1", ContractId(1), mandi.id(), farmer.id(), Performative::INFORM, payload, SimTime(0), SimTime(100)), SimTime(30));
    bus.sendAt(Envelope("MSG-2", ContractId(1), farmer.id(), mandi.id(), Performative::INFORM, payload, SimTime(0), SimTime(100)), SimTime(60));
    bus.sendAt(Envelope("MSG-3", ContractId(1), mandi.id(), farmer.id(), Performative::INFORM, payload, SimTime(0), SimTime(100)), SimTime(90));

    // Dispatch up to 60m: MSG-1 and MSG-2 delivered, MSG-3 remains in scheduler
    size_t dispatched = bus.dispatchUpTo(SimTime(60));
    CHECK(dispatched == 2);
    CHECK(clock.now().ticks() == 60);

    CHECK(farmer.receivedMessages.size() == 1);
    CHECK(farmer.receivedMessages[0] == "MSG-1");

    CHECK(mandi.receivedMessages.size() == 1);
    CHECK(mandi.receivedMessages[0] == "MSG-2");

    // Dispatch remaining
    bus.dispatchAll();
    CHECK(clock.now().ticks() == 90);
    CHECK(farmer.receivedMessages.size() == 2);
    CHECK(farmer.receivedMessages[1] == "MSG-3");
}

TEST_CASE("Determinism_ByteForByteIdenticalEventLogs", "[foundation]") {
    auto runSimulation = [](uint64_t seed) -> std::vector<std::string> {
        Scheduler scheduler;
        EventLog eventLog;
        SimClock clock;
        Rng rng(seed);
        EventBus bus(scheduler, eventLog, &clock, &rng);

        TestEchoAgent agentA(AgentId("Agent_A"));
        TestEchoAgent agentB(AgentId("Agent_B"));
        bus.registerAgent(agentA.id(), &agentA);
        bus.registerAgent(agentB.id(), &agentB);

        InformPayload payload{"data"};

        // Schedule random events
        for (int i = 0; i < 20; ++i) {
            int64_t t = rng.uniformInt(5, 500);
            std::string msgId = "MSG_" + std::to_string(i);
            AgentId src = (i % 2 == 0) ? agentA.id() : agentB.id();
            AgentId dst = (i % 2 == 0) ? agentB.id() : agentA.id();
            bus.sendAt(Envelope(msgId, ContractId(i + 1), src, dst, Performative::INFORM, payload, SimTime(0), SimTime(1000)), SimTime(t));
        }

        bus.dispatchAll();

        std::vector<std::string> logLines;
        for (const auto& entry : eventLog.entries()) {
            logLines.push_back(entry.toLogLine());
        }
        return logLines;
    };

    // Run 1 with seed 42
    auto logRun1 = runSimulation(42);

    // Run 2 with seed 42
    auto logRun2 = runSimulation(42);

    // Verify exact byte-for-byte identity
    REQUIRE(logRun1.size() == logRun2.size());
    for (size_t i = 0; i < logRun1.size(); ++i) {
        CHECK(logRun1[i] == logRun2[i]);
    }

    // Run 3 with different seed 99 must produce a different log
    auto logRun3 = runSimulation(99);
    bool anyDifference = (logRun1.size() != logRun3.size());
    if (!anyDifference) {
        for (size_t i = 0; i < logRun1.size(); ++i) {
            if (logRun1[i] != logRun3[i]) {
                anyDifference = true;
                break;
            }
        }
    }
    CHECK(anyDifference);
}
