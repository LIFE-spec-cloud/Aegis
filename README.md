# KisanSetu (MandiFlow)
### Multi-Agent Planning, Coordination and Recovery System (MAPCRS)
**Advanced Object-Oriented Programming Course Project · C++20 Reference Implementation**

---

## 1. Executive Pitch & Mission

**KisanSetu** is a discrete-event, multi-agent simulation of Indian perishable-produce logistics (tomatoes, onions, chillies). Farmers, transporters, APMC mandis, and food processing plants are modeled as autonomous agents with **private state** (reserve prices, truck margins, order books, and gate quotas). 

The system plans decentralized shipments via a broker using the **Contract Net Protocol (CNP)**, commits multi-party physical and financial resources atomically via **Two-Phase Commit (2PC)**, and recovers deterministically when disruptions occur (highway stalls, mandi price gluts, or truck engine failures). Farmers interact through simulated vernacular voice kiosks and one-click confirmation cards rather than complicated dashboards.

Every interesting behavior emerges from:
1. **Agent Interaction & Private Information:** No central optimizer can observe private reserve prices or internal vehicle states.
2. **Time-Dependent Quality Decay:** Produce deteriorates along an exponential decay curve ($Q(t) = Q_0 \cdot \exp(-\lambda \cdot t \cdot \mu_{\text{temp}})$) and maps to discrete commercial grade price steps.
3. **Compensating Rollback:** Physical commitments (truck locks, gate slots, escrow deposits) cannot be silently discarded; they must be compensated in reverse order.

---

## 2. System Architecture & Two-Person Split

The codebase is split into two cleanly separated, decoupled modules meeting at frozen interface contracts in `include/contracts/`:

```
c:/Projects/Aegis/
├── include/contracts/       # Frozen shared interface contracts
│   ├── Types.h              # Strong types: SimTime, Money (paise), Quintals, AgentId, ContractId
│   ├── Envelope.h           # Typed message envelope and performatives
│   ├── Payloads.h           # CNP, telemetry, fault, and distress payloads
│   ├── IReservationPorts.h  # ISlotPort, ITruckLockPort, IEscrowPort (Move-only RAII handles)
│   ├── ITelemetry.h         # TelemetryEvent, MandiPriceTickEvent, ITelemetryWatchdogListener
│   ├── IRecoveryContext.h   # Read-only recovery query services
│   ├── IOrderInputPort.h    # InputEvent, ConfirmationCard, and DomainCommand
│   └── EventLog.h           # Append-only structured event log
│
├── commands/                # [Person B] Commitment, RAII Guards & Undo Log
│   ├── ICommand.h           # Command pattern base
│   ├── ReservationGuards.h  # Move-only RAII reservation guards
│   ├── UndoLog.h            # Reverse-order compensation undo log
│   ├── TransitContractCommand.h / .cpp  # 2PC state machine with rebind & divert
│   └── InvariantChecker.h   # Continuous zero-leak mathematical auditor
│
├── observer/                # [Person B] Telemetry Watchdog & Audio Announcements
│   ├── TelemetryWatchdog.h / .cpp       # Observer pattern telemetry analyzer
│   └── AudioSpeakerNotifier.h / .cpp    # Spoken Hinglish voice alert synthesizer
│
├── recovery/                # [Person B] Strategy Pattern Recovery Algorithms
│   ├── IFailureRecovery.h   # Recovery strategy interface
│   ├── DistressAuctionRecovery.h / .cpp  # Processing plant distress auction (<30km)
│   ├── DynamicReroutingRecovery.h / .cpp # Alternate mandi rerouting with hysteresis (gain > ₹5000)
│   └── PeerHandoffRecovery.h / .cpp      # Mid-highway peer truck manifest transfer
│
├── ui_adapter/              # [Person B] Accessible Vernacular Kiosk Adapter
│   ├── VernacularLexicon.h / .cpp       # Hindi/Hinglish vocabulary dictionary & unit normalizer
│   ├── VoiceKioskAdapter.h / .cpp       # Adapter pattern translating speech/DTMF to domain commands
│   └── ProduceStatusCard.h  # Visual color-graded quality card (Green/Amber/Red)
│
├── simulation/              # [Person B / Co-owned] Fault Injection & Console Dashboard
│   ├── FaultInjector.h / .cpp           # Deterministic fault scheduler (seed 42)
│   ├── Dashboard.h / .cpp   # 5-panel ANSI / Plain console dashboard
│   └── main_demo.cpp        # Standalone executable demo driver
│
├── tests/                   # Test suites (100% passing under C++20)
│   ├── fakes/               # Scriptable mocks with failure injection switches
│   ├── test_commands.cpp    # 2PC rollback, idempotency, and invariant tests
│   ├── test_recovery.cpp    # Observer and 3 recovery scenario tests
│   └── test_interface.cpp   # 22 voice phrases, cards, speaker, and plain dashboard tests
│
└── docs/                    # Defense and design documentation
    ├── patterns.md          # 6 OOP patterns with necessity tests
    ├── sequence_diagrams.md # Mermaid sequence diagrams for all flows
    └── defense_qa.md        # 15 comprehensive academic defense questions & answers
```

---

## 3. The 6 Object-Oriented Design Patterns

1. **Command + Compensation** (`commands/TransitContractCommand.h`): Executes 2PC atomic commitments across truck, mandi slot, and escrow resources. Compensates in reverse order upon disruption with version-guarded idempotency.
2. **Observer** (`observer/TelemetryWatchdog.h`, `AudioSpeakerNotifier.h`): Broadcasts telemetry and disruption alerts to decoupled listeners (dashboard, speaker, recovery manager) without modifying the watchdog.
3. **Adapter** (`ui_adapter/VoiceKioskAdapter.h`): Adapts rural vernacular voice transcripts, DTMF tones, and button taps into strongly typed domain commands.
4. **Strategy** (`recovery/IFailureRecovery.h`): Encapsulates swappable recovery strategies (`DistressAuctionRecovery`, `DynamicReroutingRecovery`, `PeerHandoffRecovery`) with automatic fallback chains.
5. **State** (`CropFreshnessState`, `TruckState`, `ContractState`): Enforces state-dependent business logic and renders illegal transitions (e.g. `Spoiled` $\to$ `GradeA`) structurally unrepresentable.
6. **Mediator** (`LogisticsBroker`): Removes circular dependencies between autonomous agents by routing CNP proposals and arbitrating contract creation.

---

### Unified Build & Test Workflow
The repository provides a unified `Makefile` and `CMakeLists.txt` configured with strict compiler flags (`-std=c++20 -Wall -Wextra -Wpedantic -Werror`):

```bash
# 1. Build all executables and test suites
make

# 2. Run all 52 test suites across both Person A and Person B modules
make test

# 3. Run the complete unified simulation demonstration
make demo

# 4. Verify architectural mediator boundary (no direct agent-to-agent includes)
make check-boundaries
```

### Direct Compilation (without Make)
```bash
# Compile unified kisansetu executable
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -I. \
    src/main.cpp src/core/*.cpp src/domain/*.cpp src/strategy/*.cpp src/agents/*.cpp \
    commands/*.cpp observer/*.cpp recovery/*.cpp ui_adapter/*.cpp \
    simulation/Dashboard.cpp simulation/FaultInjector.cpp -o kisansetu

# Run interactive demo
./kisansetu --demo
```

---

## 5. Three Calibrated Fault Scenarios

Every fault is deterministic and reproducible under random seed `42`:

1. **Highway Stoppage / Spoilage Breach (Fault 1):**
   - **Math:** Tomato batch with $\lambda = 0.030$, $\mu_{\text{temp}} = 1.2$. Baseline trip 8h gives $Q = \exp(-0.288) \approx 0.75$ (Grade B). Stoppage adds 7h delay (15h total); predicted arrival quality $Q = \exp(-0.54) \approx 0.583$, breaching the $0.60$ mandi rejection floor.
   - **Action:** `DistressAuctionRecovery` searches processors within 30 km; `Tropicana_Plant_2` (15 km) wins at ₹1,050/q. Mandi gate slot at Azadpur is immediately released, and truck is diverted. Zero leaked locks.
2. **Mandi Glut / Supply Surge (Fault 2):**
   - **Event:** Azadpur price drops 40% (₹2,200 $\to$ ₹1,320), falling below the farmer's reserve (₹1,800).
   - **Action:** `DynamicReroutingRecovery` evaluates alternative mandis with hysteresis threshold $\delta = \text{INR } 5,000$ to prevent oscillation. Reroutes to Ghazipur (gain = ₹16,500 > ₹5,000). Old Azadpur slot is cancelled, Ghazipur slot reserved. Enforces bounded reroute cap ($\le 2$).
3. **Transporter Breakdown / Engine Failure (Fault 3):**
   - **Event:** Truck enters `FAILED` state mid-highway.
   - **Action:** `PeerHandoffRecovery` matches closest idle peer truck (`Truck-DL99`, 50q capacity) at highway junction. Manifest transferred, old truck lock released, peer truck locked. If no peer truck is in range, cascades to distress auction fallback.

---

## 6. Mathematical Invariant Verification

After every state transition and recovery action, `InvariantChecker::verify()` asserts:
$$\sum \text{Active Port Locks} \equiv \sum \text{Resources Owned by Non-Terminal Contracts}$$
$$\forall c \in \text{TerminalContracts } (\text{COMPLETED, COMPENSATED, ABORTED}), \quad \text{Locks}(c) = 0$$
This guarantees zero resource leaks across all execution paths.
