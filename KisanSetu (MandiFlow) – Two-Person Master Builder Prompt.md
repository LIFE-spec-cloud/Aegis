# KisanSetu (MandiFlow) – Two-Person Team Edition of the Master Builder Prompt

The work is split so each of you can build in parallel from day one, against **frozen interface contracts**, and meet at four sync points.

**How to use this doc**

1. Both of you paste the full original Master Builder Prompt into your own assistant, from ROLE through CODING STANDARDS.
2. **Ignore its BUILD PHASES and HOW TO WORK WITH ME sections.** This doc replaces them.
3. Then paste your own section below: **Person A prompt** or **Person B prompt**.
4. Agree who is A and who is B before starting.

## 1. The Split at a Glance

|  | Person A: Planning and Coordination | Person B: Commitment, Recovery and Interface |
| --- | --- | --- |
| Pillars owned | Autonomous Planning, Decentralized Coordination | Fault-Tolerant Recovery, Accessibility |
| Modules | `core/`, `domain/`, `strategy/`, `agents/` (Farmer, Transporter, Mandi, Processor, Broker) | `commands/`, `observer/`, `recovery/`, `ui_adapter/`, `simulation/` (FaultInjector, Dashboard) |
| Patterns owned | State, Strategy, Mediator | Command + compensation, Observer, Adapter |
| Core deliverables | Scheduler, EventBus, decay and grades, road graph, net-margin planner, CNP, double auction, gate quota, surge price model, parallel mode, baseline evaluation | TransitContractCommand with RAII guards and invariant checker, telemetry watchdog, three recovery strategies, fault injector, voice adapter and lexicon, audio notifier, ANSI dashboard |
| Tests owned | Decay and grade boundaries, state tables, strategy swaps, auction clearing, race test, determinism test, parallel-mode test, baseline comparison | Rollback tests at every PREPARE step, leak invariant, three fault scenario tests, voice parsing (20+ phrases), adapter fake-port tests, dashboard plain-mode snapshot |
| Docs owned | `design.md` (engine, agents, CNP sequence diagrams), `limitations.md` | `patterns.md` (necessity tests), rollback and recovery sequence diagrams, `defense_qa.md` |

Both of you co-own `README.md` and the final demo script.

## 2. Shared Contracts (the thing that makes parallel work possible)

Put these in `include/contracts/`. **A writes them first (Sync 0); B reviews and approves.** After the freeze, any change needs a pull request approved by both.

| Contract | Purpose | Used by |
| --- | --- | --- |
| `Types.h` | Strong types: `AgentId`, `ContractId`, `Money` (integer paise), `Quintals`, `SimTime`, `Hours` | Everyone |
| `Envelope.h` and `Payloads.h` | Message envelope, performatives, all payload structs (TaskAnnouncement, Cfp, Propose, Accept, Reject, Cancel, TelemetryUpdate, FailureNotice, DistressBid, …) | Everyone |
| `IReservationPorts.h` | `ISlotPort` (reserve/cancel gate slot), `ITruckLockPort` (lock/release/rebind truck), `IEscrowPort` (hold/refund/transfer deposit). Each returns a move-only handle | B consumes in `TransitContractCommand`; A implements inside broker and agents |
| `ITelemetry.h` | `TelemetryEvent` (truck position, speed, state, predicted arrival, mandi price tick) and `ITelemetryWatchdogListener` | A's agents emit; B's watchdog and dashboard consume |
| `IRecoveryContext.h` | Read-only broker services for recovery: `findIdleTrucks(location, capacity)`, `rankMandis(batch, exclude)`, `findProcessors(location, radiusKm)`, `routeTo(from, to, strategy)`, `predictQuality(batch, route)` | B's recovery uses; A implements |
| `IOrderInputPort.h` | `parse(InputEvent) -> Command` and typed domain commands (`CreateHarvestBatchCommand`, `ConfirmContractCommand`) | B writes the adapter; A's FarmerAgent consumes the commands |
| `EventLog.h` | Append-only log format (one line per event) used by tests and the dashboard | Everyone |

**Rule:** until the other person's implementation exists, code against **fakes** you write yourself in `tests/fakes/`. Never wait idle.

## 3. Working Agreement

- **Repo:** one Git repo, `main` always builds and passes tests. Feature branches named `a/<topic>` and `b/<topic>`, merged by pull request, reviewed by the other person.
- **Directory ownership:** you edit only your own modules. Touching the other person's module needs a pull request comment first.
- **Contracts folder:** changes only through a pull request approved by both.
- **CI script** (`scripts/ci.sh`): build with `-Werror`, run all tests, run under AddressSanitizer and UndefinedBehaviorSanitizer. ThreadSanitizer for A's parallel mode.
- **Daily 10-minute sync:** what is merged, what is blocked, any contract change needed.
- **Cross-review rule:** every pull request needs one review from the other person. It doubles as defense preparation, because each of you must be able to explain the whole system.

## 4. Sync Points

| Sync | Trigger | Outcome |
| --- | --- | --- |
| **S0: Contract freeze** | A pushes `include/contracts/` plus an empty buildable skeleton | Both build the skeleton; B approves contracts; both can now work independently |
| **S1: Engine and domain ready** | A finishes phases A1–A2 | B swaps selected fakes for the real Scheduler, EventBus and domain types |
| **S2: Happy path end to end** | A finishes CNP (A3) and B finishes command and rollback (B1) | One farmer, one truck, one mandi: announce, bid, award, two-phase commit, delivery, with the real command inside the real broker |
| **S3: Recovery integration** | B finishes recovery (B2) and A finishes forecast and surge (A4) | The three fault scenarios run end to end with exact expected outcomes |
| **S4: Final demo** | A5 and B3 done | `./kisansetu --demo` with dashboard, voice input, parallel mode check, baseline report, docs |

## 5. Parallel Timeline

| Stage | Person A | Person B |
| --- | --- | --- |
| Start | A0: contracts and skeleton (S0) | B0: review contracts; write fakes for engine and agents |
| Stage 1 | A1: foundation (scheduler, bus, config, Rng). A2: domain (decay, states, road graph, strategies) → **S1** | B1: TransitContractCommand, RAII guards, invariant checker, rollback tests against fakes |
| Stage 2 | A3: agents, CNP, auction, gate quota → **S2** | B2: watchdog, DistressAuctionRecovery, DynamicReroutingRecovery, PeerHandoffRecovery, FaultInjector, scenario tests against fakes |
| Stage 3 | A4: forecast, surge, herd-effect mitigation → **S3** | B3: VoiceKioskAdapter, lexicon, audio notifier, dashboard |
| Stage 4 | A5: parallel mode, message-loss option, baseline evaluation, sanitizers → **S4** | B4: docs, defense Q&A, demo script rehearsal → **S4** |

If one of you finishes early, take review work and defense Q&A first, then offer to help the other person's phase. Do not rewrite the other person's modules.

---

## 6. PERSON A PROMPT (Planning and Coordination)

Paste this after the original prompt's ROLE to CODING STANDARDS sections.

**YOUR ROLE.** You are **Person A**, the engine and coordination owner of KisanSetu. A teammate, **Person B**, builds commitment, recovery, voice adapter and dashboard in parallel. You must never block them: your first job is to freeze the contracts.

**YOU OWN:** `core/`, `domain/`, `strategy/`, `agents/`, `include/contracts/`, `config/`, build system and CI script. **YOU MUST NOT EDIT:** `commands/`, `observer/`, `recovery/`, `ui_adapter/`, `simulation/FaultInjector`, `simulation/Dashboard`. Ask Person B through a pull request comment instead.

**PHASES (stop after each and report; wait for my approval):**

- **A0 Contracts and skeleton.** Create the repo skeleton, CMake, CI script, config loader, and the full `include/contracts/` set described in Section 2 of the team doc. Provide compile-only stubs so the skeleton builds. Report and wait: this is **Sync 0**.
- **A1 Foundation.** `Rng` (seeded `mt19937_64`), `SimClock`, `Scheduler` (priority queue by simTime then sequence), `EventBus`, mailboxes, `Agent` base. Tests: event ordering, same-seed determinism.
- **A2 Domain.** `Commodity`, `CropBatch`, decay function, `CropFreshnessState` State pattern with legal-transition tables, `RoadGraph` with mutable weights, `IRoutePlanningStrategy` (Dijkstra, SpoilageMinimizing, TollAverse), `IPricingStrategy`. Tests: grade boundaries (including exactly 0.85, 0.65, 0.40), illegal transitions rejected, strategy swap changes the route. Report: this is **Sync 1**.
- **A3 Agents and CNP.** `FarmerAgent`, `TransporterAgent`, `APMCMandiAgent`, `ProcessorAgent`, `LogisticsBroker`. Implement the reservation ports (`ISlotPort`, `ITruckLockPort`, `IEscrowPort`) and the broker-run CNP. Mandi owns gate slots alone. Periodic uniform-price call auction. Tests: full CNP happy path, last-slot race with identical winner across 100 seeded runs. Together with Person B's command this is **Sync 2**.
- **A4 Planning quality.** Forecast price with surge penalty from known inbound trucks, risk-adjusted net margin, hysteresis threshold δ and reroute cap, herd-effect mitigation (stagger and penalise crowded alternatives). Implement `IRecoveryContext`. Tests: forecast avoids herding in a 10-truck reroute case. **Sync 3.**
- **A5 Hardening and evaluation.** `--parallel` mode with per-agent mailboxes (broker still serializes commits), optional message delay/drop, ThreadSanitizer run, baseline comparison (static single mandi, no recovery vs full system: average net margin, spoilage loss, salvage rate). Docs: `design.md`, `limitations.md`. **Sync 4.**

**WORKING RULES FOR YOU**

1. Start by restating your scope in 8 lines and listing assumptions. Then do **A0 only**.
2. Contracts must be minimal, immutable and documented. Any later change requires a note to Person B.
3. Every emitted `TelemetryEvent` and every contract state change goes to the `EventLog`.
4. Never invent results. State plainly what you did not run.
5. At each gate report: files added, tests passing, decisions (one line each), anything Person B must know.

---

## 7. PERSON B PROMPT (Commitment, Recovery and Interface)

Paste this after the original prompt's ROLE to CODING STANDARDS sections.

**YOUR ROLE.** You are **Person B**, owner of atomic commitment, fault recovery and the farmer-facing interface of KisanSetu. A teammate, **Person A**, builds the engine, domain, agents and CNP in parallel and will publish `include/contracts/` first. Until real implementations arrive, build against **fakes** you write in `tests/fakes/`.

**YOU OWN:** `commands/`, `observer/`, `recovery/`, `ui_adapter/`, `simulation/FaultInjector`, `simulation/Dashboard`, `tests/fakes/`. **YOU MUST NOT EDIT:** `core/`, `domain/`, `strategy/`, `agents/`, `include/contracts/` (propose changes through a pull request).

**PHASES (stop after each and report; wait for my approval):**

- **B0 Contract review.** When Person A pushes the contracts (Sync 0), review them against your needs. Write minimal fakes: fake slot, truck and escrow ports (with failure injection switches), fake telemetry source, fake recovery context with scripted trucks, mandis and processors. Report any contract gaps as a short list.
- **B1 Commitment and rollback.** `TransitContractCommand` (CREATED → PREPARING → COMMITTED → COMPLETED | COMPENSATED | ABORTED), RAII reservation guards, undo log, reverse-order idempotent `compensate()`, `abort()`, `rebind()` for reroute and handoff, invariant checker (reserved resources equal resources owned by non-terminal contracts). Tests: force a failure at each PREPARE step, call `compensate()` twice, rebind slot only, rebind truck only. Together with Person A's CNP this is **Sync 2**.
- **B2 Telemetry and recovery.** `TelemetryWatchdog` (Observer) with `ITelemetryWatchdogListener`; `IFailureRecovery`; `DistressAuctionRecovery` (processors within 30 km, highest net bid), `DynamicReroutingRecovery` (hysteresis δ, reroute cap, uses `rankMandis`), `PeerHandoffRecovery` (closest idle truck, rendezvous minimising added decay, fallback chain: finishing truck, cold storage, distress auction); `FaultInjector` scripted at fixed sim times with seed 42. Scenario tests assert the exact event sequence and end state for the three faults, using calibrated values from the master prompt. **Sync 3** with real agents.
- **B3 Interface.** `IOrderInputPort`, `VoiceKioskAdapter`, `VernacularLexicon` (Hindi/Hinglish: quantity, units, crops, intents), confidence score, `ConfirmationCard`, produce status card (green, amber, red), `AudioSpeakerNotifier` (Observer), ANSI `Dashboard` with `--no-color`, `--plain` and `--step` modes. Tests: 20+ voice phrases including noisy ones, adapter used with a fake engine, dashboard plain-mode snapshot.
- **B4 Docs and demo.** `patterns.md` (each pattern, where it lives, its failing-without test), rollback and recovery sequence diagrams (Mermaid), `defense_qa.md` (15 likely questions), demo script. **Sync 4.**

**WORKING RULES FOR YOU**

1. Start by restating your scope in 8 lines and listing assumptions. Then do **B0 only** (if contracts are not pushed yet, tell me and draft the fakes from the contract list in Section 2 of the team doc).
2. Never edit Person A's code to make your tests pass. Report the mismatch.
3. Voice input alone must never commit a contract; low confidence yields a confirmation card.
4. Never invent results. State plainly what you did not run.
5. At each gate report: files added, tests passing, decisions (one line each), anything Person A must know.

---

## 8. Integration Checklist (do together at S2, S3, S4)

- [ ] `main` builds clean with `-Werror`; all tests green under AddressSanitizer and UndefinedBehaviorSanitizer.
- [ ] S2: happy path run leaves zero leaked reservations (invariant checker passes after every event).
- [ ] S3: each of the three faults reproduces the exact expected end state from seed 42, twice in a row, byte-identical event logs.
- [ ] S3: recovery rollback leaves no leaked slot, truck lock or escrow.
- [ ] S4: `./kisansetu --demo` shows the live dashboard with faults in red and recoveries in green.
- [ ] S4: parallel mode ledger equals deterministic ledger; ThreadSanitizer clean.
- [ ] S4: baseline report generated; docs complete; both of you can answer any defense question about the whole system.

## 9. Risks of Working in Parallel

| Risk | Mitigation |
| --- | --- |
| Contracts change after freeze and break the other person | Pull-request approval from both; version comment at the top of each contract header |
| Fakes drift from real behaviour | Run B's tests against real implementations at S2 and S3; fix fakes if they diverge |
| One person finishes much earlier | Take over reviews, defense Q&A and test hardening, not the other's modules |
| Merge conflicts in shared files (`CMakeLists.txt`, config) | A owns them; B requests additions by pull request |
| Uneven understanding at the defense | Cross-review every pull request and swap a 10-minute walkthrough of your module each week |
