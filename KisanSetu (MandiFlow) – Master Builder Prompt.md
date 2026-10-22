# KisanSetu (MandiFlow) – Master Builder Prompt

Paste everything below the line into a coding assistant (Claude Code, Cursor, or a chat model). It builds the project phase by phase, with a stop-and-report gate after each phase.

---

## ROLE

You are a Principal Software Architect and Senior C++20 engineer specializing in distributed systems, concurrency and object-oriented design. You will build **KisanSetu (MandiFlow)**, the Multi-Agent Planning, Coordination and Recovery System (MAPCRS), as a graded advanced-OOP course project. Quality bar: production-grade, deterministic, testable, defense-ready.

## MISSION

Build a discrete-event, multi-agent simulation of Indian APMC perishable-produce logistics. Farmers, transporters, mandis and processing plants are autonomous agents with **private state**. They plan, negotiate through a mediator, commit to physical contracts atomically, and **recover deterministically** from three injected faults. Farmers interact through a simulated vernacular voice and one-click interface. A live ANSI console dashboard shows everything.

This is NOT a CRUD or management system. Every behaviour must emerge from agent messages, time-dependent decay, and compensating rollback.

## HARD RULES

1. **Language and tooling:** C++20, CMake 3.20+, GCC 11+/Clang 14+. Standard library only for the engine. Catch2 or GoogleTest is allowed for tests only. No other third-party dependencies.
2. **Determinism:** the reference mode is a single-threaded discrete-event scheduler. Same seed gives the same event log, byte for byte. No wall-clock time, no `std::random_device`; use a seeded `std::mt19937_64` owned by the simulation.
3. **No agent holds a pointer or reference to another agent.** Agents interact only through typed messages routed by the event bus or the LogisticsBroker (Mediator). Identify agents by `AgentId` (strong typedef).
4. **Ownership:** `std::unique_ptr` for ownership, `std::shared_ptr` only where shared ownership is genuinely required (justify in a comment). No raw owning pointers, no `new`/`delete` in user code.
5. **Immutability:** messages and value objects are immutable (`const` members or `const` after construction). Mutable state is private to its owning agent.
6. **RAII for resources:** every reservation (truck lock, gate slot, escrow deposit) is held by an RAII guard that releases on scope exit unless committed.
7. **No cheating on patterns:** each pattern below must be necessary, not decorative, and each must have a test that fails without it.
8. **Do not skip phases.** After each phase: build, run all tests, print a short report (what exists, what passes, what is deferred), then **stop and wait for my approval**.
9. When you must choose between two designs, choose the simpler one, write the reason in a one-line comment, and list it in the phase report.

## DOMAIN PARAMETERS (all in `config/default.cfg`, loaded at startup)

Simulated time unit: 1 tick = 1 simulated minute. Currency: ₹ per quintal. Volume: quintals.

**Quality decay:** Q(t) = Q0 · exp(−λ · t · μ\_temp), Q0 = 1.0, t in hours.

| Parameter | Tomato | Onion | Chilli |
| --- | --- | --- | --- |
| λ (per hour) | 0.030 | 0.008 | 0.020 |
| Base price ₹/quintal | 2200 | 1800 | 6000 |

- μ\_temp: 1.0 for 20 °C or below, rising linearly to 1.5 at 40 °C; refrigerated truck option multiplies μ\_temp by 0.6 and adds freight cost.
- **Grade thresholds:** A ≥ 0.85, B ≥ 0.65, C ≥ 0.40, Spoiled below 0.40. Price multipliers: 1.00 / 0.75 / 0.45 / 0.00.
- `mandi_min_quality = 0.60` (mandis reject below this; processors accept down to 0.40).

**Net margin:** NetMargin(m) = P\_m(t\_arrival) · Q(t\_transit) · Volume − TransitCost(d\_m) − MandiFees\_m. Planning uses the *forecast* P\_m (current quote minus the expected surge penalty from known inbound trucks) and subtracts a risk term: risk\_aversion · Var(delay). Use the grade price multiplier for pricing the arriving Q.

**Surge price model:** P = P\_base · (1 − k · max(0, arrivals / absorption\_capacity − 1)), clamped to a floor of 0.4 · P\_base.

**Demo world (defaults):** 4 mandis (Azadpur, Ghazipur, Vashi, Kolar), 2 processing plants, 6 trucks, 5 farmers, a road graph with at least 12 nodes (farm clusters, junctions, mandis, plants). Gate quota 4 trucks/hour per mandi. Edge weights (time, cost, toll, congestion) are mutable at runtime.

**Auction style:** periodic uniform-price call auction (sealed bids collected for a window, cleared once per window at the market-clearing price), with the surge model applied to the clearing price.

## ARCHITECTURE

```
agro_multiagent/
├── CMakeLists.txt
├── config/default.cfg
├── include/ and src/ mirroring these modules:
│   ├── core/        Agent, AgentId, Message, Envelope, EventBus, Scheduler, SimClock, Rng
│   ├── domain/      Commodity, CropBatch, CropFreshnessState, RoadGraph, PriceModel
│   ├── agents/      FarmerAgent, TransporterAgent, APMCMandiAgent, ProcessorAgent, LogisticsBroker
│   ├── strategy/    IRoutePlanningStrategy (Dijkstra, SpoilageMinimizing, TollAverse), IPricingStrategy
│   ├── commands/    ICommand, TransitContractCommand, reservation guards
│   ├── recovery/    IFailureRecovery, DistressAuctionRecovery, DynamicReroutingRecovery, PeerHandoffRecovery
│   ├── observer/    ITelemetryWatchdogListener, TelemetryWatchdog, AudioSpeakerNotifier
│   ├── ui_adapter/  IOrderInputPort, VoiceKioskAdapter, VernacularLexicon
│   └── simulation/  MainSimulation, FaultInjector, Dashboard
├── tests/
└── docs/            design notes, class diagram (Mermaid), defense Q&A
```

### Core engine

- `Scheduler`: priority queue of events ordered by (simTime, sequenceNumber). Single-threaded reference loop pops the next event and delivers it to the target agent's mailbox, then runs that agent's `onMessage`. No global state.
- `Envelope`: `messageId`, `correlationId` (contract id), `sender`, `receiver`, `performative` (CFP, PROPOSE, ACCEPT, REJECT, CANCEL, INFORM, FAILURE), `payload` (a `std::variant` of typed payload structs), `sentAt`, `ttl`. Immutable.
- Optional **parallel mode** (flag `--parallel`): agents on a thread pool with per-agent mailboxes (mutex-protected queues), but the broker still serializes contract commits. It must reproduce the same final contract ledger as deterministic mode on the three scenarios.
- **Deadlock avoidance:** reservations always acquired in global order truck → gate slot → deposit; every reservation has a TTL; never hold one lock while waiting for another agent's reply.
- Optional message delay/drop in the EventBus (config), with at-least-once delivery and idempotent handlers.

### Agents and private state

| Agent | Private state | Does |
| --- | --- | --- |
| FarmerAgent | reserve price, deposit budget, risk aversion, crop batch | Announces tasks, ranks bundles by net margin, awards, re-plans on recovery |
| TransporterAgent | cost/km, fuel, mechanical health, true ETA, position | Bids, drives the road graph, emits telemetry, signals STALLED / FAILED |
| APMCMandiAgent | order book, buyer reserves, true yard congestion, gate slots | Quotes price, owns slots (sole authority), clears auction, applies surge model |
| ProcessorAgent | intake capacity, price floor | Bids in distress auctions |
| LogisticsBroker (Mediator) | contract ledger, escrow | Runs CNP, two-phase commit, escrow, routes recovery |

Each agent owns its state; there is no shared mutable state between agents.

### Design patterns (mandatory, each with a necessity test)

| Pattern | Implementation | Test that fails without it |
| --- | --- | --- |
| State | `CropFreshnessState` (GradeA, GradeB, GradeC, Spoiled) and `TruckState` (IDLE, EN\_ROUTE, STALLED, FAILED), with legal-transition tables | Illegal transitions (Spoiled → GradeA, FAILED → EN\_ROUTE) are rejected |
| Strategy | `IRoutePlanningStrategy`, `IPricingStrategy`, swappable at runtime | Same scenario, different strategy, different route/price, no agent code changed |
| Mediator | `LogisticsBroker` | No agent header includes another agent's header (enforced by a CMake include check or a grep test) |
| Command + compensation | `TransitContractCommand` with `execute()`, `compensate()`, `abort()` | Fault tests leave zero leaked locks |
| Adapter | `VoiceKioskAdapter` implements `IOrderInputPort` | Engine tests run with a fake input port; the engine never sees raw strings |
| Observer | `ITelemetryWatchdogListener`; watchdog, dashboard, recovery manager and audio notifier subscribe | Adding a listener needs no change to the watchdog |

## COORDINATION PROTOCOL

**Tripartite Contract Net Protocol (broker-managed):**

1. Farmer sends `TaskAnnouncement` (crop, volume, ready time, deadline) to the broker.
2. Broker fans out CFP to transporters (freight + ETA) and to shortlisted mandis (gate slot + forecast price).
3. Each party replies with a PROPOSE derived from its private state, with an expiry (ttl). Stale proposals are rejected.
4. Farmer ranks (transporter, mandi) bundles by risk-adjusted net margin and sends ACCEPT.
5. Broker runs **two-phase commit** inside `TransitContractCommand`: PREPARE (reserve truck, gate slot, deposit in that order) then COMMIT. Any failure triggers `compensate()` for what was reserved.

**Race rule:** the mandi agent is the sole owner of gate slots. Two farmers requesting the last slot are serialized by the mandi's mailbox order (simTime, sequence). The loser gets REJECT and re-plans. Add a test where this happens and the winner is identical across 100 runs.

## TransitContractCommand (spec)

- State machine: CREATED → PREPARING → COMMITTED → (COMPLETED | COMPENSATED | ABORTED).
- `execute()`: acquires reservations via RAII guards in the fixed order, records each acquisition in an undo log, moves to COMMITTED only if all succeed.
- `compensate()`: releases in **reverse** order of the undo log, refunds the unused escrow share, cancels the gate slot, frees the truck lock; **idempotent** (guarded by a contract version number; calling twice changes nothing).
- `abort()`: used before COMMIT; releases whatever was reserved so far.
- The command also supports **rebind** (for reroute/handoff): compensates only the affected resource (slot or truck) and re-reserves the replacement, keeping the contract id.
- Every state change is written to the append-only event log.
- Invariant checked after every event in tests: reserved resources = resources owned by non-terminal contracts.

## FAULT SCENARIOS (deterministic; seed 42; scripted by `FaultInjector`)

Calibrated reference values (verify with unit tests; adjust only via config and report any change):

**Fault 1: Highway stoppage / spoilage breach.** Tomato, μ\_temp 1.2, so λ·μ = 0.036/h. Baseline trip 8 h gives Q = 0.75 (Grade B, sells at a mandi). Injected stall adds 7 h, so predicted arrival at 15 h gives Q ≈ 0.583, below `mandi_min_quality` 0.60. Detection: watchdog predicts Q\_arrival < 0.60. Recovery `DistressAuctionRecovery`: processors within 30 km bid, the highest net bid wins; broker compensates the gate slot, truck is rebound to the plant. Expected end state: truck delivers to the plant, sale at processing price, zero leaked locks.

**Fault 2: Mandi glut.** Azadpur quote ₹2200; 50 unannounced trucks arrive, price falls 40% to ₹1320, below the farmer's reserve ₹1800. Recovery `DynamicReroutingRecovery`: recompute edge weights, re-run planning over remaining mandis (with surge forecast to avoid herding), reroute only if gain > δ (default ₹5,000 total) and reroute count < 2. Expected: truck diverted to the best alternate mandi, old slot cancelled, new slot reserved.

**Fault 3: Transporter breakdown.** Truck enters FAILED mid-highway (or misses heartbeat for 3 ticks). Recovery `PeerHandoffRecovery`: choose the closest idle truck with enough capacity, select the rendezvous point minimising added decay, transfer the manifest, rebind the contract. If handoff net margin is worse than distress sale, fall back to distress auction. If no idle truck is in range: nearest truck finishing a trip, then cold storage, then distress auction.

**Recovery principles:** idempotent, hysteresis, bounded attempts, every step logged, each scenario has exactly one expected outcome.

## VERNACULAR VOICE / ONE-CLICK ADAPTER

- `IOrderInputPort`: `Command parse(InputEvent)`. `InputEvent` = `VoiceText` (simulated transcript), `Dtmf`, or `ButtonTap`.
- `VoiceKioskAdapter`: rule-based parser with a `VernacularLexicon` (Hindi/Hinglish): quantity words and digits, units (crate, quintal, kg normalised to quintals with a configurable crate weight), crops (tamatar, pyaaz, mirch), intents (bechna hai, bhejna hai, rok do).
- Example: `"50 crates tamatar bechna hai"` → `CreateHarvestBatchCommand{crop=Tomato, qty=..., intent=Sell}` with confidence score.
- Confidence below threshold produces a `ConfirmationCard` (one-tap green tick) instead of acting. Voice alone never commits a contract.
- `AudioSpeakerNotifier` (Observer): turns recovery decisions into short Hinglish sentences, printed with a speaker marker in the console.
- Produce status card: Green = Grade A (sell), Amber = Grade B/C (at risk, dispatch now), Red = Spoiled.

## CONSOLE DASHBOARD (ANSI, refreshes per sim step)

Panels, in order: header with sim clock and seed; truck lanes (route progress bar, state, cargo, ETA); crop freshness meters (green ≥ 0.85, amber 0.40–0.85, red < 0.40, with Q value and grade); mandi price tickers (price, arrow, glut flag, slot occupancy); recovery event log (last 8 events; injected faults and recovery actions in **bold red**, successful recoveries in green). Provide `--no-color` and `--plain` modes so output can be diffed in tests, and a `--step` mode for demos.

## TESTING REQUIREMENTS

1. Unit tests: decay function and grade boundaries, state transition tables, strategy swaps, voice parsing (at least 20 phrases including noisy ones), mandi surge model, auction clearing.
2. Scenario tests: the three faults, asserting the exact event sequence and end state.
3. Race test: last-slot contention, identical winner across 100 seeded runs.
4. Rollback tests: force failure at each step of PREPARE and assert no leaked resources.
5. Determinism test: two runs with the same seed give identical event logs.
6. Parallel mode test: final ledger equals deterministic ledger.
7. Baseline comparison: static single-mandi, no-recovery run vs full system; report average net margin, spoilage loss, salvage rate.
8. Run with AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer (parallel mode) in CI scripts.

## BUILD PHASES (stop for approval after each)

1. **Foundation:** CMake, config loader, Rng, SimClock, Scheduler, EventBus, Envelope, AgentId. Tests for ordering and determinism.
2. **Domain:** Commodity, decay, CropFreshnessState (State pattern), RoadGraph with mutable weights, strategies (Dijkstra, SpoilageMinimizing, TollAverse), PriceModel.
3. **Agents and CNP:** Farmer, Transporter, Mandi, Processor, Broker; CNP end to end; double auction and gate quota; net-margin planning.
4. **Commands and rollback:** TransitContractCommand, RAII guards, 2PC, invariant checker, rollback and race tests.
5. **Telemetry and recovery:** watchdog (Observer), the three recovery strategies, FaultInjector, scenario tests.
6. **Adapter and dashboard:** VoiceKioskAdapter, lexicon, confirmation cards, AudioSpeakerNotifier, ANSI dashboard.
7. **Hardening and evaluation:** parallel mode, optional message loss, sanitizers, baseline comparison, final report.

## DOCUMENTATION DELIVERABLES

- `README.md`: build, run, flags, demo script (the three faults in order).
- `docs/design.md`: architecture, Mermaid class diagram and sequence diagrams (CNP, rollback, each recovery).
- `docs/patterns.md`: each pattern, where it lives, why it is necessary, its failing-without test.
- `docs/defense_qa.md`: 15 likely examiner questions with answers (why not a single LLM, why multi-agent, race conditions, unrealistic assumptions, herd effect, determinism vs threads, Vickrey vs call auction).
- `docs/limitations.md`: honest list of assumptions (perfect forecast noise level, lossless messaging by default, simulated voice).

## CODING STANDARDS

- `-Wall -Wextra -Wpedantic -Werror`, clang-format file, no compiler warnings.
- Prefer `enum class`, `std::variant` + `std::visit` for messages, `std::optional`, concepts for strategy constraints, `[[nodiscard]]` on queries, `final` on leaf classes, `override` always.
- Public interfaces in headers with brief doc comments; no logic in headers except templates and trivial inline.
- Strong types for ids, money (₹, integer paise), quantity (quintals), and simTime. Never pass bare `int`/`double` where a domain type exists.
- Every `catch` and every error path is handled and logged; no silent failures.
- No global mutable state or singletons.

## HOW TO WORK WITH ME

1. Begin by restating your understanding of the project in 10 lines and listing any assumption you are making. Ask at most three questions, only if truly blocking.
2. Then execute **Phase 1 only**. Show the file tree, key interfaces, test results.
3. At each gate, report: files added, tests passing, design decisions made (one line each), risks, and what the next phase will do. Wait for "approved" before continuing.
4. If a requirement here conflicts with correctness, stop and tell me instead of silently deviating.
5. Never invent results: if a test fails or you did not run something, say so.

**Definition of done:** all seven phases approved; all tests green under sanitizers; the three fault scenarios run end to end from `./kisansetu --demo` with a live dashboard; documentation deliverables complete; the demo is reproducible from a single seed.

---

End of prompt. Begin with Section HOW TO WORK WITH ME, step 1.
