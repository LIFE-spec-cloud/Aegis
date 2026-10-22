# KisanSetu (MandiFlow) – Object-Oriented Design Patterns

In KisanSetu, design patterns are **never decorative**. Every pattern serves a direct structural purpose in managing information asymmetry, physical contention, irreversible commitments, or accessibility. Each pattern has a dedicated necessity test that demonstrably fails if the pattern is removed.

---

## Pattern Matrix

| Pattern | Where it Lives | Purpose in KisanSetu | Failing-Without Necessity Test |
| :--- | :--- | :--- | :--- |
| **Command + Compensation** | `commands/TransitContractCommand.h`, `UndoLog.h`, `ReservationGuards.h` | Atomic 2-Phase Commit across distributed physical and financial resources (truck, gate slot, escrow) with reverse-order idempotent rollback. | `testFailureAtPrepareStep2_SlotReservation` & `testFailureAtPrepareStep3_EscrowHold` in `tests/test_commands.cpp`. Without the command undo log, partial failures leak truck locks and gate slots, causing `InvariantChecker` to throw a fatal resource leak assertion. |
| **Observer** | `observer/TelemetryWatchdog.h`, `ITelemetryWatchdogListener.h`, `AudioSpeakerNotifier.h` | Broadcasts telemetry streams, transit delay alerts, and market price shocks to decoupled subscribers (dashboard, recovery engine, voice speaker) without coupling the telemetry source to specific consumers. | `testObserverPatternNecessity` in `tests/test_recovery.cpp`. Verifies that registering or removing listeners dynamically alters notification dispatch with zero modifications to the core `TelemetryWatchdog` source code. |
| **Adapter** | `ui_adapter/VoiceKioskAdapter.h`, `IOrderInputPort.h`, `VernacularLexicon.h` | Translates untyped, noisy vernacular inputs (Hindi/Hinglish transcripts, DTMF keys, kiosk button taps) into strongly typed domain commands (`CreateHarvestBatchCommand`, `ConfirmContractCommand`). | `testVernacularVoiceParsing20PlusPhrases` and `testButtonTapAndDtmf` in `tests/test_interface.cpp`. Verifies that the domain simulation core never ingests raw text or strings, and works with a fake input port interchangeably. |
| **Strategy** | `recovery/IFailureRecovery.h`, `DistressAuctionRecovery.h`, `DynamicReroutingRecovery.h`, `PeerHandoffRecovery.h` | Allows dynamic swapping of recovery algorithms at runtime depending on the nature of the disruption (spoilage breach, market glut, mechanical breakdown) and multi-tier fallback chains. | `testScenario4_PeerHandoff_FallbackChain` in `tests/test_recovery.cpp`. Verifies that when peer handoff has no idle trucks in range, the system seamlessly delegates to `DistressAuctionRecovery` without modifying contract or agent code. |
| **State** | `CropFreshnessState` (GradeA $\to$ GradeB $\to$ GradeC $\to$ Spoiled), `TruckState` (IDLE, EN_ROUTE, STALLED, FAILED), `ContractState` | Encapsulates state-dependent behaviour (price multipliers, recovery eligibility) and makes invalid transitions (e.g. `Spoiled` $\to$ `GradeA`, `FAILED` $\to$ `EN_ROUTE`) compile-time or runtime unrepresentable. | Tested in `tests/test_commands.cpp` and `tests/test_recovery.cpp`. Attempting to transition a `COMPENSATED` contract or execute a terminal command fails immediately; attempting to deliver spoiled tomatoes at an APMC gate slot triggers automated rejection. |
| **Mediator** | `LogisticsBroker` | Coordinates the Contract Net Protocol (CNP) and 2PC between Farmers, Transporters, and Mandis. Eliminates $O(N^2)$ circular references between autonomous agents. | Structural architectural rule: no agent header includes another agent header (e.g., `FarmerAgent` never includes `TransporterAgent` or `APMCMandiAgent`). All interactions are mediated through typed messages routed by the broker. |

---

## Deep Dive: Pattern Implementations

### 1. Command + Compensation (`commands/TransitContractCommand.h`)
- **Problem:** Agricultural contracts require acquiring physical resources that cannot be rolled back via SQL `ROLLBACK` (a truck is physically reserved; an APMC gate slot is locked; farmer money is deposited in escrow). If the escrow hold fails after the truck and slot are reserved, the system must deterministically undo the prior commitments in reverse order.
- **Solution:** `TransitContractCommand` tracks state transitions (`CREATED` $\to$ `PREPARING` $\to$ `COMMITTED` $\to$ `COMPLETED` | `COMPENSATED` | `ABORTED`). As each resource is acquired, an undo step is appended to `UndoLog`. If prepare fails or if an external disruption requires rollback, `compensate()` executes the undo log in reverse order:
  $$\text{Acquisition: } \text{Truck} \to \text{Slot} \to \text{Escrow} \quad \Longrightarrow \quad \text{Compensation: } \text{Escrow} \to \text{Slot} \to \text{Truck}$$
- **Idempotency:** Guarded by `m_version`. Calling `compensate()` twice does not execute double-refunds or double-releases.

### 2. Observer (`observer/TelemetryWatchdog.h`)
- **Problem:** In a fast-moving logistics network, truck speeds, highway delays, and mandi price crashes must be monitored continuously. Hardcoding the recovery manager, console dashboard, and vernacular audio speaker into the telemetry producer would create tight coupling and violate the Open/Closed Principle.
- **Solution:** `ITelemetryWatchdogListener` defines pure virtual hooks (`onTruckTelemetry`, `onMandiPriceTick`, `onAnomalyAlert`). Any component can subscribe to `TelemetryWatchdog`. When an anomaly is detected (e.g., $Q_{\text{arrival}} < 0.60$), the watchdog broadcasts the `AnomalyAlert` to all registered listeners simultaneously.

### 3. Adapter (`ui_adapter/VoiceKioskAdapter.h`)
- **Problem:** Smallholder Indian farmers interact using local dialects (Hindi/Hinglish) and voice kiosks (*"50 crates tamatar bechna hai"*). The discrete-event simulation engine cannot be polluted with natural language processing or string-parsing heuristics.
- **Solution:** `VoiceKioskAdapter` implements `IOrderInputPort`. It tokenizes vernacular phrases against `VernacularLexicon`, calculates a confidence score, normalizes local packaging units (crates, kg) to standardized `Quintals`, and outputs typed domain commands. If confidence is below threshold, it generates a `ConfirmationCard` requiring an explicit green tick button tap.

### 4. Strategy (`recovery/IFailureRecovery.h`)
- **Problem:** Different physical disruptions require radically different domain solutions:
  - Spoilage breach requires a local processing auction (ketchup plant within 30 km).
  - Mandi market crash requires dynamic rerouting to an alternative mandi with hysteresis.
  - Truck engine breakdown requires mid-highway peer truck rendezvous and manifest transfer.
- **Solution:** `IFailureRecovery` provides a polymorphic `recover()` method. Each strategy encapsulates its own business rules, allowing the recovery engine to switch strategies or execute fallback cascades dynamically.
