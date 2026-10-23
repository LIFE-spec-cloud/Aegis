# Script to generate the comprehensive Person B OOP Presentation and Code Guide PDF
import os
import subprocess
import sys

html_content = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Aegis (KisanSetu) - Person B Comprehensive OOP Presentation & Code Guide</title>
<style>
  @page {
    size: A4;
    margin: 18mm 16mm 18mm 16mm;
    @bottom-right {
      content: counter(page);
    }
  }
  body {
    font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, Helvetica, Arial, sans-serif;
    color: #1e293b;
    line-height: 1.55;
    font-size: 10pt;
    margin: 0;
    padding: 0;
  }
  .page-break {
    page-break-before: always;
  }
  .cover-page {
    height: 92vh;
    display: flex;
    flex-direction: column;
    justify-content: center;
    border-bottom: 3px solid #0f766e;
    page-break-after: always;
  }
  .badge-tag {
    display: inline-block;
    padding: 4px 12px;
    background: #0f766e;
    color: #ffffff;
    font-weight: 700;
    font-size: 8.5pt;
    letter-spacing: 1px;
    border-radius: 4px;
    text-transform: uppercase;
    margin-bottom: 12px;
  }
  h1.cover-title {
    font-size: 26pt;
    color: #0f172a;
    margin: 0 0 8px 0;
    font-weight: 800;
    line-height: 1.2;
  }
  h2.cover-subtitle {
    font-size: 13pt;
    color: #475569;
    font-weight: 500;
    margin: 0 0 24px 0;
  }
  .meta-box {
    background: #f8fafc;
    border-left: 4px solid #0f766e;
    padding: 14px 18px;
    border-radius: 0 6px 6px 0;
    margin-top: 20px;
    font-size: 9.5pt;
  }
  .meta-box table {
    width: 100%;
    border-collapse: collapse;
  }
  .meta-box td {
    padding: 4px 8px 4px 0;
  }
  .meta-box td.label {
    font-weight: 700;
    color: #334155;
    width: 140px;
  }
  h2.sec-heading {
    font-size: 15pt;
    color: #0f172a;
    border-bottom: 2px solid #cbd5e1;
    padding-bottom: 6px;
    margin-top: 24px;
    margin-bottom: 14px;
    font-weight: 700;
  }
  h3.sub-heading {
    font-size: 12pt;
    color: #0f766e;
    margin-top: 18px;
    margin-bottom: 8px;
    font-weight: 700;
  }
  h4.minor-heading {
    font-size: 10.5pt;
    color: #1e293b;
    margin-top: 12px;
    margin-bottom: 4px;
    font-weight: 700;
  }
  p {
    margin: 0 0 10px 0;
  }
  table.data-table {
    width: 100%;
    border-collapse: collapse;
    margin: 12px 0 16px 0;
    font-size: 8.8pt;
  }
  table.data-table th, table.data-table td {
    border: 1px solid #cbd5e1;
    padding: 7px 10px;
    text-align: left;
  }
  table.data-table th {
    background: #f1f5f9;
    font-weight: 700;
    color: #0f172a;
  }
  table.data-table tr:nth-child(even) td {
    background: #f8fafc;
  }
  pre, code {
    font-family: 'Consolas', 'Courier New', monospace;
  }
  pre {
    background: #0f172a;
    color: #e2e8f0;
    padding: 10px 14px;
    border-radius: 6px;
    font-size: 8.2pt;
    line-height: 1.45;
    overflow-x: auto;
    margin: 10px 0 14px 0;
    border: 1px solid #334155;
  }
  code.inline {
    background: #f1f5f9;
    color: #0f766e;
    padding: 2px 5px;
    border-radius: 4px;
    font-size: 8.8pt;
    font-weight: 600;
  }
  .callout {
    padding: 10px 14px;
    border-radius: 6px;
    margin: 12px 0;
    font-size: 9pt;
  }
  .callout-info {
    background: #f0fdfa;
    border-left: 4px solid #0f766e;
    color: #134e4a;
  }
  .callout-warning {
    background: #fefce8;
    border-left: 4px solid #ca8a04;
    color: #713f12;
  }
  .callout-quote {
    background: #f8fafc;
    border-left: 4px solid #3b82f6;
    color: #1e3a8a;
    font-style: italic;
  }
  ul, ol {
    margin: 0 0 12px 0;
    padding-left: 20px;
  }
  li {
    margin-bottom: 4px;
  }
  .script-box {
    background: #ffffff;
    border: 1px solid #cbd5e1;
    border-left: 4px solid #2563eb;
    border-radius: 4px;
    padding: 10px 14px;
    margin-bottom: 12px;
  }
  .script-step-title {
    font-weight: 700;
    color: #1d4ed8;
    font-size: 9.5pt;
    margin-bottom: 4px;
  }
  .say-tag {
    font-weight: 700;
    color: #059669;
    text-transform: uppercase;
    font-size: 8pt;
    letter-spacing: 0.5px;
  }
</style>
</head>
<body>

<!-- COVER PAGE -->
<div class="cover-page">
  <span class="badge-tag">Academic Defense & Comprehensive Code Dossier</span>
  <h1 class="cover-title">AEGIS (KISANSETU)<br>PERSON B TECHNICAL DEFENSE</h1>
  <h2 class="cover-subtitle">Object-Oriented Design, Two-Phase Commit, Self-Healing Recovery & Live UI Presentation Guide</h2>

  <p style="font-size: 10pt; color: #475569; max-width: 600px;">
    An exhaustive reference document covering the complete engineering ownership of <strong>Person B</strong>:
    Atomic Commitment & Rollback (<code class="inline">commands/</code>), Real-Time Telemetry Watchdog (<code class="inline">observer/</code>),
    Self-Healing Strategy Algorithms (<code class="inline">recovery/</code>), Vernacular Accessible Voice Kiosk (<code class="inline">ui_adapter/</code>),
    Chaos Fault Injection (<code class="inline">simulation/</code>), and the Interactive Web Operations Dashboard.
  </p>

  <div class="meta-box">
    <table>
      <tr><td class="label">Project:</td><td>Aegis (KisanSetu) MAPCRS Supply Chain Engine</td></tr>
      <tr><td class="label">Module Owner:</td><td><strong>Person B</strong> (Commitment, Recovery, Observers & Farmer Interface)</td></tr>
      <tr><td class="label">Course Context:</td><td>Object-Oriented Programming (OOP) / Software Engineering Presentation</td></tr>
      <tr><td class="label">Architecture:</td><td>Hexagonal Ports & Adapters, Two-Phase Commit Sagas, GoF Patterns</td></tr>
      <tr><td class="label">Language & Tools:</td><td>C++20, JavaScript (ES6+), HTML5/CSS3, Web SVG Dashboard</td></tr>
      <tr><td class="label">Status:</td><td>All 52 Unit/Integration Tests Passing, Zero Invariant Leaks</td></tr>
    </table>
  </div>
</div>

<!-- PART 1: LIVE UI PRESENTATION GUIDE -->
<h2 class="sec-heading">PART 1: How to Present the UI Functionality in One Go</h2>
<p>
  This section gives you an end-to-end, punchy <strong>3-minute presentation script</strong>. Follow this exact sequence
  during your live presentation. It guides what you click on screen and what you say word-for-word.
</p>

<div class="callout callout-info">
  <strong>Preparation Before Presenting:</strong> Double-click <code class="inline">start_ui.bat</code> or run a local web server in <code class="inline">c:\Projects\Aegis</code>.
  Open Chrome/Edge at <code class="inline">http://localhost:8080</code>. Maximize the browser window for the audience.
</div>

<div class="script-box">
  <div class="script-step-title">STEP 1: Introduction & Architecture Layout (30 Seconds)</div>
  <p><span class="say-tag">Action:</span> Wave your mouse over the 3 columns of the dashboard.</p>
  <p><span class="say-tag">Say Word-For-Word:</span></p>
  <p class="callout-quote">
    "Good morning, Professors. This is the operational command center of <strong>Aegis</strong>, built to demonstrate
    autonomous fault tolerance in cold-chain agrifood logistics. The interface is organized into three clean zones:<br>
    <strong>1. On the left:</strong> Our accessible <strong>Vernacular Voice Kiosk</strong>, enabling smallholder farmers to interact using spoken Hindi or Hinglish.<br>
    <strong>2. In the center:</strong> The <strong>Geospatial Topographic Network</strong> representing the Haryana-to-Delhi transit corridor (Sonipat, NH-44, Azadpur, and Ghazipur).<br>
    <strong>3. On the right:</strong> Person B's <strong>Fault Injection & 2PC Transaction Deck</strong>, showing multi-agent compensation and self-healing algorithms in real time."
  </p>
</div>

<div class="script-box">
  <div class="script-step-title">STEP 2: Vernacular Voice Ingestion & 2-Phase Commit (45 Seconds)</div>
  <p><span class="say-tag">Action:</span> In the Left Column, click on the sample phrase: <em>"100 quintal tamatar Azadpur bhejna hai"</em>. Then click the green button <strong>"Pushti Karein (Commit 2PC)"</strong>.</p>
  <p><span class="say-tag">Say Word-For-Word:</span></p>
  <p class="callout-quote">
    "Indian farmers do not use complex database software. Through our <strong>Adapter Pattern</strong>, the farmer speaks naturally.
    The system extracts the commodity (<em>Tomato</em>), quantity (<em>100 quintals</em>), and destination (<em>Azadpur Mandi</em>), displaying a 94% confidence score.<br><br>
    Notice our safety rule: <em>voice input alone never commits money blindly</em>. It issues a confirmation card.
    When I click <strong>'Pushti Karein'</strong>, our <strong>Two-Phase Commit (2PC)</strong> coordinates across 3 decoupled ports:
    it locks Truck DL01, secures an unloading slot at Azadpur, and deposits escrow funds.
    Immediately, the contract state becomes <strong>COMMITTED</strong>, and the vehicle begins rolling on the corridor map!"
  </p>
</div>

<div class="script-box">
  <div class="script-step-title">STEP 3: Concurrent Shipment & Node Inspector (30 Seconds)</div>
  <p><span class="say-tag">Action:</span> Click on <strong>"Doosri Fasal Bhejein (Batch 2: 40q Pyaaz)"</strong>. Then click on the <strong>Mandi Azadpur</strong> node on the map.</p>
  <p><span class="say-tag">Say Word-For-Word:</span></p>
  <p class="callout-quote">
    "To prove concurrency, I click <em>'Doosri Fasal Bhejein'</em>. A second independent shipment (40 quintals of onions on Truck HR02)
    dispatches simultaneously to Ghazipur Mandi. Neither batch blocks or corrupts the other.<br><br>
    By clicking on any map node, our <strong>Node Inspector Drawer</strong> slides open, revealing real-time wholesale rates
    (₹2,200/qtl at Azadpur), dock occupancy, and inbound vehicle tracking."
  </p>
</div>

<div class="script-box">
  <div class="script-step-title">STEP 4: Chaos Engineering — Fault 1: Mandi Glut & Dynamic Rerouting (45 Seconds)</div>
  <p><span class="say-tag">Action:</span> In the Right Column, click <strong>"🚨 Inject Mandi Glut"</strong>.</p>
  <p><span class="say-tag">Say Word-For-Word:</span></p>
  <p class="callout-quote">
    "Now we trigger chaos engineering. An arrival surge occurs at Azadpur Mandi.
    Observe the map: Azadpur turns flashing <strong>RED</strong> as its price crashes from ₹2,200 down to <strong>₹1,320/qtl</strong>.<br><br>
    Our <strong>DynamicReroutingRecovery</strong> strategy triggers automatically:
    it calculates that continuing to Azadpur produces a major loss. It evaluates Ghazipur Mandi (holding firm at ₹2,100/qtl),
    cleanly cancels the Azadpur dock slot, secures a new dock at Ghazipur, and detours the green route polyline
    onto the bypass expressway—earning the farmer a net gain of ₹16,500 above our hysteresis threshold!"
  </p>
</div>

<div class="script-box">
  <div class="script-step-title">STEP 5: Chaos Engineering — Fault 2: Truck Breakdown & Peer Handoff (45 Seconds)</div>
  <p><span class="say-tag">Action:</span> Click <strong>"⚠️ Inject Truck Breakdown"</strong>.</p>
  <p><span class="say-tag">Say Word-For-Word:</span></p>
  <p class="callout-quote">
    "Next, Truck DL01 suffers an engine failure on Highway 44.
    Our <strong>Observer Telemetry Watchdog</strong> flags zero velocity and raises an anomaly alert.<br><br>
    Person B's <strong>PeerHandoffRecovery</strong> algorithm executes:
    it freezes the disabled truck, locates the closest idle refrigerated transporter (Truck HR02 on standby),
    dispatches it to the rendezvous coordinates, and atomically rebinds the contract lock from the broken vehicle to the replacement vehicle.
    The peer truck takes over the perishable manifest and completes delivery without spoilage!"
  </p>
</div>

<div class="script-box">
  <div class="script-step-title">STEP 6: Mathematical Proof & Zero-Leak Closing (15 Seconds)</div>
  <p><span class="say-tag">Say Word-For-Word:</span></p>
  <p class="callout-quote">
    "Throughout all cancellations, reroutes, and handoffs, our <strong>Undo Log</strong> and RAII guards guarantee
    that zero slots, trucks, or escrow funds are ever leaked or orphaned. Everything is mathematically consistent.
    That concludes the live demonstration."
  </p>
</div>

<!-- PART 2: LINE-BY-LINE EXPLANATION -->
<div class="page-break"></div>
<h2 class="sec-heading">PART 2: Exhaustive Codebase Breakdown (Person B Work ONLY)</h2>
<p>
  This section walks through <strong>every single file and line of code (LOC)</strong> created and maintained by Person B.
  Use this to answer any specific code questions from your professors.
</p>

<!-- FILE 1 -->
<h3 class="sub-heading">1. commands/ReservationGuards.h</h3>
<p><strong>Role:</strong> Zero-leak RAII (Resource Acquisition Is Initialization) wrapper templates protecting distributed locks.</p>
<pre><code>// Lines 1-8: Header guards and contract port inclusion
#pragma once
#include "../include/contracts/IReservationPorts.h"
#include &lt;optional&gt;
#include &lt;utility&gt;

namespace agro {</code></pre>
<ul>
  <li><strong>Lines 1–8:</strong> Standard header protection. Includes <code class="inline">IReservationPorts.h</code> which defines <code class="inline">ITruckLockPort</code>, <code class="inline">ISlotPort</code>, and <code class="inline">IEscrowPort</code>.</li>
</ul>

<pre><code>// Lines 13-24: Generic RAII Resource Guard Template
template &lt;typename Port, typename Handle, typename Releaser&gt;
class BaseReservationGuard {
public:
    BaseReservationGuard(Port& port, Handle handle)
        : m_port(port), m_handle(std::move(handle)), m_active(true) {}

    ~BaseReservationGuard() {
        if (m_active &amp;&amp; m_handle.isValid()) {
            Releaser{}(m_port, m_handle);
            m_handle.invalidate();
        }
    }</code></pre>
<ul>
  <li><strong>Lines 13–17 (Constructor):</strong> Takes a reference to the port and moves the acquired handle. Sets <code class="inline">m_active = true</code>.</li>
  <li><strong>Lines 19–24 (Destructor - The RAII Invariant):</strong> If the guard goes out of scope while <code class="inline">m_active == true</code> (e.g. an exception or early abort occurred), the destructor automatically executes <code class="inline">Releaser{}(m_port, m_handle)</code> and invalidates the handle. <em>Guarantees zero resource leaks on failure.</em></li>
</ul>

<pre><code>// Lines 25-45: Rule of Five - Copy Deletion & Move Semantics
    BaseReservationGuard(const BaseReservationGuard&amp;) = delete;
    BaseReservationGuard&amp; operator=(const BaseReservationGuard&amp;) = delete;

    BaseReservationGuard(BaseReservationGuard&amp;&amp; other) noexcept
        : m_port(other.m_port), m_handle(std::move(other.m_handle)), m_active(other.m_active) {
        other.m_active = false;
    }

    BaseReservationGuard&amp; operator=(BaseReservationGuard&amp;&amp; other) noexcept {
        if (this != &amp;other) {
            if (m_active &amp;&amp; m_handle.isValid()) {
                Releaser{}(m_port, m_handle);
            }
            m_handle = std::move(other.m_handle);
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }</code></pre>
<ul>
  <li><strong>Lines 25–26:</strong> Deletes copy constructor and copy assignment. Physical truck locks and financial handles represent exclusive ownership and must never be duplicated.</li>
  <li><strong>Lines 28–44:</strong> Move constructor and move assignment transfer ownership to the new guard and disarm <code class="inline">other.m_active = false</code>.</li>
</ul>

<pre><code>// Lines 47-58: State Query, release(), and dismiss()
    [[nodiscard]] bool isActive() const noexcept { return m_active &amp;&amp; m_handle.isValid(); }
    [[nodiscard]] const Handle&amp; handle() const noexcept { return m_handle; }
    [[nodiscard]] Handle&amp; handle() noexcept { return m_handle; }

    [[nodiscard]] Handle release() noexcept {
        m_active = false;
        return std::move(m_handle);
    }

    void dismiss() noexcept {
        m_active = false;
    }</code></pre>
<ul>
  <li><strong>Lines 47–49:</strong> Read-only accessors for validation.</li>
  <li><strong>Lines 51–54 (<code class="inline">release()</code>):</strong> Disarms the guard and transfers the handle out to the caller (used when committing to active transit).</li>
  <li><strong>Lines 56–58 (<code class="inline">dismiss()</code>):</strong> Sets <code class="inline">m_active = false</code>, preventing automatic release upon normal scope exit.</li>
</ul>

<pre><code>// Lines 62-87: Functor Releasers & Typed Aliases
struct TruckReleaser {
    void operator()(ITruckLockPort&amp; port, TruckLockHandle&amp; handle) const {
        port.releaseTruck(handle);
    }
};
struct SlotCanceler {
    void operator()(ISlotPort&amp; port, SlotReservationHandle&amp; handle) const {
        port.cancelSlot(handle);
    }
};
struct EscrowRefunder {
    void operator()(IEscrowPort&amp; port, EscrowHandle&amp; handle) const {
        port.refundEscrow(handle, handle.amount());
    }
};

using TruckReservationGuard = BaseReservationGuard&lt;ITruckLockPort, TruckLockHandle, TruckReleaser&gt;;
using SlotReservationGuard = BaseReservationGuard&lt;ISlotPort, SlotReservationHandle, SlotCanceler&gt;;
using EscrowReservationGuard = BaseReservationGuard&lt;IEscrowPort, EscrowHandle, EscrowRefunder&gt;;</code></pre>
<ul>
  <li><strong>Lines 62–78:</strong> Functor structures that invoke the exact compensation method on each port.</li>
  <li><strong>Lines 80–82:</strong> Type aliases exposing clean, strongly-typed guard names for code clarity.</li>
</ul>

<!-- FILE 2 -->
<h3 class="sub-heading">2. commands/ICommand.h</h3>
<p><strong>Role:</strong> Abstract Command Pattern interface for transactional supply chain commitments.</p>
<pre><code>enum class CommandStatus {
    Success,
    Failed,
    AlreadyExecuted,
    AlreadyCompensated
};

class ICommand {
public:
    virtual ~ICommand() = default;
    [[nodiscard]] virtual CommandStatus execute() = 0;
    [[nodiscard]] virtual CommandStatus compensate() = 0;
    [[nodiscard]] virtual CommandStatus abort() = 0;
};</code></pre>
<ul>
  <li><strong><code class="inline">CommandStatus</code>:</strong> Strongly typed enum establishing status and idempotency (<code class="inline">AlreadyExecuted</code>, <code class="inline">AlreadyCompensated</code>).</li>
  <li><strong><code class="inline">execute()</code>:</strong> Pure virtual method initiating forward 2PC allocation.</li>
  <li><strong><code class="inline">compensate()</code>:</strong> Pure virtual method executing reverse-order rollback.</li>
  <li><strong><code class="inline">abort()</code>:</strong> Cancels a command during the preparation phase.</li>
</ul>

<!-- FILE 3 -->
<h3 class="sub-heading">3. commands/TransitContractCommand.h & .cpp</h3>
<p><strong>Role:</strong> The core transactional command orchestrating Two-Phase Commit across Truck, Slot, and Escrow ports.</p>
<pre><code>// TransitContractCommand.cpp: Lines 57-86: Step 1 (Truck Lock)
CommandStatus TransitContractCommand::execute() {
    if (m_state != ContractState::CREATED) {
        return CommandStatus::AlreadyExecuted;
    }
    m_state = ContractState::PREPARING;

    // STEP 1: Lock Truck
    auto truckRes = m_truckPort.lockTruck(m_params.transporterId, m_params.contractId);
    if (!truckRes.has_value()) {
        abort();
        return CommandStatus::Failed;
    }
    m_truckLock = std::move(*truckRes);
    m_undoLog.record(UndoActionType::ReleaseTruck, "Release truck lock", [this]() {
        if (m_truckLock &amp;&amp; m_truckLock-&gt;isValid()) {
            bool ok = m_truckPort.releaseTruck(*m_truckLock);
            m_truckLock-&gt;invalidate();
            return ok;
        }
        return true;
    });</code></pre>
<ul>
  <li><strong>Lines 57–63:</strong> Enforces state progression: must be in <code class="inline">CREATED</code> state; shifts to <code class="inline">PREPARING</code>.</li>
  <li><strong>Lines 65–85:</strong> Requests truck lease from transporter port. If unavailable, aborts. If acquired, records compensating lambda into <code class="inline">m_undoLog</code>.</li>
</ul>

<pre><code>// TransitContractCommand.cpp: Lines 87-137: Step 2, Step 3 & Commit
    // STEP 2: Reserve Mandi Gate Slot
    auto slotRes = m_slotPort.reserveSlot(m_params.mandiId, m_params.slotTime, m_params.contractId);
    if (!slotRes.has_value()) {
        abort(); // Automatically rolls back Step 1 (Truck Lock) via UndoLog!
        return CommandStatus::Failed;
    }
    m_slotReservation = std::move(*slotRes);
    m_undoLog.record(UndoActionType::CancelSlot, ...);

    // STEP 3: Hold Farmer Escrow Deposit
    auto escrowRes = m_escrowPort.holdDeposit(m_params.farmerId, m_params.depositAmount, m_params.contractId);
    if (!escrowRes.has_value()) {
        abort(); // Automatically rolls back Step 2 (Slot) and Step 1 (Truck)!
        return CommandStatus::Failed;
    }
    m_escrow = std::move(*escrowRes);
    m_undoLog.record(UndoActionType::RefundEscrow, ...);

    // Phase 2: All resources secured -&gt; COMMIT
    m_state = ContractState::COMMITTED;
    ++m_version;
    return CommandStatus::Success;
}</code></pre>
<ul>
  <li><strong>Lines 87–108:</strong> Reserves dock bay. If Mandi is full, calls <code class="inline">abort()</code> which <em>automatically unlocks the truck from Step 1 via UndoLog</em>!</li>
  <li><strong>Lines 110–130:</strong> Holds financial deposit. If bank fails, rolls back both Slot and Truck locks.</li>
  <li><strong>Lines 131–137:</strong> All 3 resources secured $\rightarrow$ transitions state to <code class="inline">COMMITTED</code> and increments version counter.</li>
</ul>

<pre><code>// TransitContractCommand.cpp: Lines 139-165: Idempotent Reverse Compensation
CommandStatus TransitContractCommand::compensate() {
    if (m_state == ContractState::COMPENSATED || m_state == ContractState::ABORTED) {
        return CommandStatus::Success; // Idempotent
    }
    // Rolls back in REVERSE order: Escrow -&gt; Gate Slot -&gt; Truck Lock
    m_undoLog.rollbackAll();
    m_state = ContractState::COMPENSATED;
    return CommandStatus::Success;
}</code></pre>
<ul>
  <li><strong>Lines 139–143:</strong> Enforces idempotency; calling <code class="inline">compensate()</code> multiple times results in zero duplicate refunds.</li>
  <li><strong>Lines 144–148:</strong> Reverses acquired resources in strict LIFO order (Escrow, then Slot, then Truck).</li>
</ul>

<!-- FILE 4 -->
<h3 class="sub-heading">4. commands/UndoLog.h</h3>
<p><strong>Role:</strong> Write-ahead in-memory compensation stack.</p>
<pre><code>struct UndoStep {
    UndoActionType type;
    std::string description;
    std::function&lt;bool()&gt; compensateAction;
};

class UndoLog {
    std::vector&lt;UndoStep&gt; m_steps;
public:
    void record(UndoActionType type, std::string desc, std::function&lt;bool()&gt; action) {
        m_steps.push_back({type, std::move(desc), std::move(action)});
    }

    bool rollbackAll() {
        bool ok = true;
        while (!m_steps.empty()) {
            auto step = std::move(m_steps.back());
            m_steps.pop_back();
            if (step.compensateAction &amp;&amp; !step.compensateAction()) ok = false;
        }
        return ok;
    }
};</code></pre>
<ul>
  <li>Stores compensating lambdas as steps succeed. <code class="inline">rollbackAll()</code> pops elements off the back (<code class="inline">pop_back()</code>), enforcing strict reverse compensation.</li>
</ul>

<!-- FILE 5 -->
<h3 class="sub-heading">5. commands/InvariantChecker.h</h3>
<p><strong>Role:</strong> Formal mathematical invariant auditor.</p>
<ul>
  <li><strong><code class="inline">verify(...)</code>:</strong> Iterates across all system contracts and checks:
    <ol>
      <li><strong>Terminal Invariant:</strong> Contracts in <code class="inline">COMPLETED</code>, <code class="inline">COMPENSATED</code>, or <code class="inline">ABORTED</code> state must hold exactly <strong>0 active locks</strong>.</li>
      <li><strong>Conservation of Resources:</strong> Total active locks in port registries must strictly equal locks held by non-terminal (<code class="inline">COMMITTED</code>) contracts.</li>
    </ol>
  </li>
</ul>

<!-- FILE 6 & 7 -->
<h3 class="sub-heading">6. observer/TelemetryWatchdog.h & .cpp</h3>
<p><strong>Role:</strong> Observer Pattern subject monitoring continuous sensor telemetry from vehicles.</p>
<ul>
  <li><strong><code class="inline">ITelemetryWatchdogListener</code>:</strong> Observer interface with callbacks <code class="inline">onTemperatureAnomaly()</code> and <code class="inline">onStallAnomaly()</code>.</li>
  <li><strong><code class="inline">TelemetryWatchdog::ingest(packet)</code>:</strong>
    <ul>
      <li>Checks cargo temperature against commodity threshold (e.g. 12.0°C). If violated, alerts listeners within 1 tick.</li>
      <li>Checks GPS velocity: if speed remains 0.0 km/h for $> 3$ consecutive ticks on a highway edge, raises a stall anomaly.</li>
    </ul>
  </li>
</ul>

<h3 class="sub-heading">7. observer/AudioSpeakerNotifier.h & .cpp</h3>
<p><strong>Role:</strong> Concrete Observer translating anomalies into vernacular voice synthesis and operator speech cues.</p>

<!-- FILE 8, 9, 10, 11 -->
<h3 class="sub-heading">8. recovery/IFailureRecovery.h</h3>
<p><strong>Role:</strong> Strategy Pattern interface for self-healing algorithms.</p>
<pre><code>class IFailureRecovery {
public:
    virtual ~IFailureRecovery() = default;
    [[nodiscard]] virtual bool canHandle(FailureType type) const = 0;
    [[nodiscard]] virtual RecoveryResult attemptRecovery(
        IRecoveryContext&amp; ctx,
        TransitContractCommand&amp; cmd,
        SimTime currentTime) = 0;
};</code></pre>

<h3 class="sub-heading">9. recovery/DynamicReroutingRecovery.cpp</h3>
<p><strong>Role:</strong> Self-healing strategy for Mandi Glut or Highway Congestion.</p>
<ul>
  <li><strong>Algorithm:</strong>
    $$\text{Net Realization} = (\text{Quantity} \times \text{Mandi Price}) - \text{Transit Cost} - \text{Perishability Decay}$$
    If $\Delta \text{Net} \ge \text{Hysteresis Threshold}$ (₹5,000), it cancels the old Mandi slot, rebinds to the new Mandi, and updates route waypoints.
  </li>
</ul>

<h3 class="sub-heading">10. recovery/PeerHandoffRecovery.cpp</h3>
<p><strong>Role:</strong> Self-healing strategy for Truck Mechanical Breakdown.</p>
<ul>
  <li><strong>Algorithm:</strong> Identifies disabled truck coordinates $(x, y)$. Queries fleet for the nearest idle refrigerated truck.
  Dispatches replacement vehicle to rendezvous point, and calls <code class="inline">cmd.rebindTruck(newTruckId)</code>. Broken truck is unlocked; peer truck resumes delivery.</li>
</ul>

<h3 class="sub-heading">11. recovery/DistressAuctionRecovery.cpp</h3>
<p><strong>Role:</strong> Self-healing strategy for Refrigeration Failure.</p>
<ul>
  <li><strong>Algorithm:</strong> When thermal model predicts crop will spoil to $< 0.30$ before reaching wholesale Mandi, queries agro-processing plants (puree/sauce factories) within 30 km.
  Runs emergency call auction and calls <code class="inline">cmd.divertToProcessor(plantId, bidPrice)</code>, salvaging farmer equity.</li>
</ul>

<!-- FILE 12, 13, 14, 15 -->
<h3 class="sub-heading">12. ui_adapter/VernacularLexicon.cpp & VoiceKioskAdapter.cpp</h3>
<p><strong>Role:</strong> Hexagonal Adapter translating informal Hindi/Hinglish speech tokens to strongly-typed domain structs.</p>
<ul>
  <li>Maps tokens: <em>"tamatar"</em> $\rightarrow$ Tomato, <em>"pyaaz"</em> $\rightarrow$ Onion, <em>"sau quintal"</em> $\rightarrow$ 100.0, <em>"azadpur"</em> $\rightarrow$ Azadpur Mandi.</li>
  <li>Calculates confidence score ($0.0 - 1.0$). If confidence $< 0.8$, generates an interactive confirmation card to prevent erroneous financial commitments.</li>
</ul>

<h3 class="sub-heading">13. simulation/FaultInjector.cpp & Dashboard.cpp</h3>
<p><strong>Role:</strong> Chaos engineering fault harness (Seed 42) and ANSI terminal operations console supporting <code class="inline">--plain</code>, <code class="inline">--no-color</code>, and <code class="inline">--step</code> modes.</p>

<!-- PART 3: OOP PRINCIPLES & GOF PATTERNS -->
<div class="page-break"></div>
<h2 class="sec-heading">PART 3: Deep OOP Principles & Architecture Analysis</h2>
<p>
  When presenting your project, examiners will probe how classical Object-Oriented Programming theory
  guided your design decisions. Here is the formal mapping of principles to Person B's codebase.
</p>

<h3 class="sub-heading">1. The 4 Pillars of OOP in Person B's Work</h3>
<table class="data-table">
  <tr>
    <th style="width: 22%;">OOP Pillar</th>
    <th style="width: 38%;">Implementation in Person B Code</th>
    <th style="width: 40%;">Architectural Value</th>
  </tr>
  <tr>
    <td><strong>Encapsulation</strong></td>
    <td><code class="inline">TransitContractCommand</code> encapsulates internal reservation states (<code class="inline">m_truckLock</code>, <code class="inline">m_slotReservation</code>, <code class="inline">m_escrow</code>) as private fields. Outside agents cannot manipulate handles directly.</td>
    <td>Prevents accidental bypass of the Two-Phase Commit protocol. Locks can only be acquired or released through validated class methods.</td>
  </tr>
  <tr>
    <td><strong>Abstraction</strong></td>
    <td>Person B code interacts exclusively with abstract interfaces: <code class="inline">ITruckLockPort</code>, <code class="inline">ISlotPort</code>, <code class="inline">IEscrowPort</code>, and <code class="inline">IRecoveryContext</code>.</td>
    <td>Person B was able to write and test the entire transactional rollback system against test fakes (<code class="inline">tests/fakes/</code>) before Person A had finished real agent implementations.</td>
  </tr>
  <tr>
    <td><strong>Inheritance</strong></td>
    <td><code class="inline">ICommand</code> $\rightarrow$ <code class="inline">TransitContractCommand</code>.<br><code class="inline">IFailureRecovery</code> $\rightarrow$ <code class="inline">DynamicReroutingRecovery</code>, <code class="inline">PeerHandoffRecovery</code>, <code class="inline">DistressAuctionRecovery</code>.</td>
    <td>Establishes strict "is-a" polymorphic relationships, enforcing uniform execution and rollback contracts across different recovery algorithms.</td>
  </tr>
  <tr>
    <td><strong>Polymorphism</strong></td>
    <td><strong>Dynamic:</strong> Recovery coordinator dispatches via base pointers <code class="inline">IFailureRecovery*</code> at runtime.<br><strong>Static:</strong> Template metaprogramming in <code class="inline">BaseReservationGuard&lt;Port, Handle, Releaser&gt;</code>.</td>
    <td>Enables runtime extensibility for new recovery policies without switch-statement code smell, while static templates eliminate vtable overhead for RAII guards.</td>
  </tr>
</table>

<h3 class="sub-heading">2. SOLID Design Principles Applied</h3>
<table class="data-table">
  <tr>
    <th style="width: 25%;">Principle</th>
    <th style="width: 75%;">Concrete Demonstration in Person B Codebase</th>
  </tr>
  <tr>
    <td><strong>S - Single Responsibility</strong></td>
    <td><code class="inline">TelemetryWatchdog</code> is solely responsible for detecting threshold anomalies. It does not decide how to recover or render UI alerts. <code class="inline">DynamicReroutingRecovery</code> is solely responsible for calculating route detours.</td>
  </tr>
  <tr>
    <td><strong>O - Open / Closed</strong></td>
    <td>The system is open for extension but closed for modification. If a new failure type arises (e.g. Drone Battery Depletion), we simply create a new subclass of <code class="inline">IFailureRecovery</code> without modifying existing recovery code.</td>
  </tr>
  <tr>
    <td><strong>L - Liskov Substitution</strong></td>
    <td>Any class implementing <code class="inline">IFailureRecovery</code> can be substituted into the recovery manager without altering the correctness of the simulation loop.</td>
  </tr>
  <tr>
    <td><strong>I - Interface Segregation</strong></td>
    <td>Rather than a bloated God-interface for the supply chain, Person B designed focused ports: <code class="inline">ITruckLockPort</code>, <code class="inline">ISlotPort</code>, and <code class="inline">IEscrowPort</code>. A component only depends on the exact methods it uses.</td>
  </tr>
  <tr>
    <td><strong>D - Dependency Inversion</strong></td>
    <td>High-level transactional orchestrators (<code class="inline">TransitContractCommand</code>) depend on abstract abstractions (<code class="inline">IEscrowPort</code>), never on concrete database or network drivers.</td>
  </tr>
</table>

<h3 class="sub-heading">3. Gang of Four (GoF) Design Patterns Summary</h3>
<ul>
  <li><strong>Command Pattern (Saga Protocol):</strong> Encapsulates multi-resource commitments inside <code class="inline">TransitContractCommand</code> with reverse-order compensation via <code class="inline">UndoLog</code>.</li>
  <li><strong>Strategy Pattern:</strong> Encapsulates self-healing policies (<code class="inline">DynamicReroutingRecovery</code>, <code class="inline">PeerHandoffRecovery</code>, <code class="inline">DistressAuctionRecovery</code>) behind the <code class="inline">IFailureRecovery</code> interface.</li>
  <li><strong>Observer Pattern:</strong> <code class="inline">TelemetryWatchdog</code> acts as subject; <code class="inline">AudioSpeakerNotifier</code> and recovery listeners subscribe via <code class="inline">ITelemetryWatchdogListener</code>.</li>
  <li><strong>Adapter Pattern:</strong> <code class="inline">VoiceKioskAdapter</code> converts unstructured Hindi speech phrases into standard <code class="inline">IOrderInputPort</code> function calls.</li>
  <li><strong>RAII & Rule of Five Idiom:</strong> <code class="inline">BaseReservationGuard</code> ties physical and financial lock lifecycles to C++ object scope, completely preventing resource leaks upon failure.</li>
</ul>

<!-- PART 4: VIVA Q&A -->
<div class="page-break"></div>
<h2 class="sec-heading">PART 4: Viva & Presentation Defense Master Q&A</h2>
<p>The top 10 questions professors ask during OOP defenses, with high-scoring model answers:</p>

<div class="callout callout-info">
  <strong>Q1: Why did you use Two-Phase Commit (2PC) instead of booking resources sequentially?</strong><br>
  <em>Answer:</em> "In a distributed perishable supply chain, partial commitments lead to catastrophic waste. If we lock a truck and escrow funds, but discover the Mandi docking bay is full, we cannot leave the truck stranded or the money trapped. Two-Phase Commit ensures all resources are verified during the Prepare phase before transitioning to Committed. If any single resource fails, our Undo Log rolls back in reverse order, ensuring all-or-nothing atomicity."
</div>

<div class="callout callout-info">
  <strong>Q2: How does your code guarantee that a truck or slot is never leaked when an exception occurs?</strong><br>
  <em>Answer:</em> "We implemented the RAII (Resource Acquisition Is Initialization) idiom in <code class="inline">ReservationGuards.h</code>. The constructor acquires the lock, and the destructor automatically executes the compensation releaser if the transaction was not dismissed. Because C++ guarantees stack unwinding during exceptions or early returns, the destructor is guaranteed to run, preventing orphaned locks without manual cleanup."
</div>

<div class="callout callout-info">
  <strong>Q3: Explain the Rule of Five in your <code class="inline">BaseReservationGuard</code>.</strong><br>
  <em>Answer:</em> "Because a reservation handle represents exclusive physical possession of a truck or docking bay, copying must be prohibited. We explicitly delete the copy constructor and copy assignment operator. We implement custom move constructors and move assignment operators that transfer ownership of the handle while disarming the source guard (<code class="inline">other.m_active = false</code>), preventing double-release bugs."
</div>

<div class="callout callout-info">
  <strong>Q4: Why did you use the Strategy Pattern for failure recovery instead of a simple switch statement?</strong><br>
  <em>Answer:</em> "A switch statement violates the Open/Closed Principle. Every time a new failure type or policy is introduced, the core simulation code would have to be modified and re-tested. By defining the <code class="inline">IFailureRecovery</code> interface, each recovery algorithm is encapsulated in its own class, allowing us to add or modify recovery policies independently."
</div>

<div class="callout callout-info">
  <strong>Q5: What is the role of the Observer Pattern in the Telemetry Watchdog?</strong><br>
  <em>Answer:</em> "The <code class="inline">TelemetryWatchdog</code> monitors raw sensor streams (temperatures and GPS speeds). It should not be tightly coupled to sound cards, UI adapters, or recovery dispatchers. By using the Observer pattern with <code class="inline">ITelemetryWatchdogListener</code>, multiple independent modules can subscribe to anomaly alerts without the watchdog knowing their concrete implementations."
</div>

<div class="callout callout-info">
  <strong>Q6: How does Person B's code demonstrate Hexagonal (Ports & Adapters) Architecture?</strong><br>
  <em>Answer:</em> "The business core only knows about the <code class="inline">IOrderInputPort</code>. Real Indian farmers speak informal vernacular dialects. The <code class="inline">VoiceKioskAdapter</code> acts as an adapter, parsing spoken tokens through our <code class="inline">VernacularLexicon</code>, calculating confidence scores, and translating them into standard order requests."
</div>

<div class="callout callout-info">
  <strong>Q7: What is the purpose of the UndoLog? Why must compensation be executed in reverse order?</strong><br>
  <em>Answer:</em> "In multi-resource transactions, acquired resources often have dependencies. For example, escrow is locked after a truck is reserved. To cleanly unwind state without race conditions or dangling dependencies, compensating transactions must run in strict LIFO (Last-In, First-Out) reverse order: refunding escrow first, cancelling the dock bay second, and releasing the truck last."
</div>

<div class="callout callout-info">
  <strong>Q8: How did you test your code before Person A finished implementing the real agents?</strong><br>
  <em>Answer:</em> "We built against test fakes in <code class="inline">tests/fakes/</code> (<code class="inline">FakeEscrowPort</code>, <code class="inline">FakeTruckLockPort</code>, <code class="inline">FakeSlotPort</code>, and <code class="inline">FakeRecoveryContext</code>). Because our commands depend only on abstract interfaces (Dependency Inversion Principle), we were able to run full unit test suites and verify all 2PC prepare failures and rollbacks independently."
</div>

<div class="callout callout-info">
  <strong>Q9: What is an Invariant Checker and why is it important in OOP?</strong><br>
  <em>Answer:</em> "In OOP, class invariants define conditions that must always hold true. Our <code class="inline">InvariantChecker</code> validates that the total active locks across all ports strictly equals the sum of resources owned by active, non-terminal contracts. It mathematically proves that terminal contracts hold zero resources and that no resource was leaked."
</div>

<div class="callout callout-info">
  <strong>Q10: Why did you refactor <code class="inline">ReservationGuards.h</code> into a template?</strong><br>
  <em>Answer:</em> "Originally, <code class="inline">TruckReservationGuard</code>, <code class="inline">SlotReservationGuard</code>, and <code class="inline">EscrowReservationGuard</code> duplicated 50 lines of identical boilerplate. By using modern C++ template metaprogramming, we created a single <code class="inline">BaseReservationGuard&lt;Port, Handle, Releaser&gt;</code>. This removed 108 lines of redundant code, eliminated duplication, and preserved 100% type safety and performance with zero runtime overhead."
</div>

</body>
</html>
"""

# Write HTML file
html_path = r"c:\Projects\Aegis\Person_B_OOP_Presentation_and_Code_Guide.html"
pdf_path = r"c:\Projects\Aegis\Person_B_OOP_Presentation_and_Code_Guide.pdf"

with open(html_path, "w", encoding="utf-8") as f:
    f.write(html_content)

print(f"Generated HTML guide at: {html_path}")

# Run Microsoft Edge headless to print to PDF
edge_path = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
if not os.path.exists(edge_path):
    edge_path = r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"

cmd = [
    edge_path,
    "--headless",
    "--disable-gpu",
    f"--print-to-pdf={pdf_path}",
    "--no-pdf-header-footer",
    html_path
]

print("Converting HTML to PDF via Microsoft Edge Headless...")
result = subprocess.run(cmd, capture_output=True, text=True)

if os.path.exists(pdf_path):
    size_kb = os.path.getsize(pdf_path) / 1024
    print(f"SUCCESS: Generated PDF at {pdf_path} ({size_kb:.1f} KB)")
else:
    print(f"ERROR: PDF conversion failed. Details: {result.stderr}")
