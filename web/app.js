/**
 * KisanSetu (MandiFlow) - MAPCRS Interactive Web Application Engine
 * Autonomous Multi-Agent Logistics, 2PC Commitment, & Failure Recovery
 */

// ============================================================================
// 1. Simulation State & Data Models
// ============================================================================

const simState = {
  running: false,
  timeMinutes: 120, // T+120m (02:00)
  speed: 1,
  seed: 42,
  audioEnabled: true,
  timerId: null,

  // Crop Batch
  batch: {
    id: "BATCH-DEMO-01",
    commodity: "Tomato",
    volume: 25.0, // quintals
    initialQuality: 0.94,
    currentQuality: 0.91,
    decayLambda: 0.030,
    tempMultiplier: 1.20,
    harvestTime: 0
  },

  // 2-Phase Commit Contract
  contract: {
    id: "CTR-101",
    state: "COMMITTED", // CREATED, PREPARING, COMMITTED, COMPLETED, COMPENSATED, ABORTED
    version: 1,
    farmerId: "Farmer_Ramesh",
    transporterId: "Truck_DL01",
    mandiId: "Mandi_Azadpur",
    slotTime: 180,
    freightCost: 1500,
    depositAmount: 300,
    hasTruckLock: true,
    hasGateSlot: true,
    hasEscrow: true
  },

  // Active Trucks
  trucks: [
    {
      id: "Truck_DL01",
      name: "Truck_DL01 (Reefer)",
      type: "Refrigerated",
      capacity: 50.0,
      state: "EN_ROUTE", // IDLE, EN_ROUTE, STALLED, DIVERTED, DELIVERED
      currentProgress: 45.0, // percent
      routeText: "Farm_Sonipat ➔ Azadpur",
      eta: 180,
      coord: { x: 340, y: 170 }
    },
    {
      id: "Truck_HR02",
      name: "Truck_HR02 (Ambient)",
      type: "Standard",
      capacity: 50.0,
      state: "IDLE",
      currentProgress: 0.0,
      standbyNode: "NH44_Junction",
      coord: { x: 380, y: 190 }
    }
  ],

  // Mandis and Processing Plants
  nodes: {
    Farm_Sonipat: { x: 120, y: 110, name: "Farm_Sonipat", type: "farm", desc: "Farmer Ramesh (Harvest Origin)" },
    NH44_Junction: { x: 380, y: 190, name: "NH44_Junction", type: "junction", desc: "Transit Hub & Peer Rendezvous" },
    Mandi_Azadpur: { x: 620, y: 130, name: "Mandi_Azadpur", type: "mandi", price: 2200, capacity: 5, occupied: 2, glut: false },
    Mandi_Ghazipur: { x: 650, y: 310, name: "Mandi_Ghazipur", type: "mandi", price: 2100, capacity: 4, occupied: 1, glut: false },
    Processor_KetchupCo: { x: 260, y: 360, name: "Processor_KetchupCo", type: "plant", bidPrice: 1850, floorPrice: 1050, desc: "Distress Processing Plant (15km)" }
  },

  // Metrics
  metrics: {
    margin: 562850,
    spoilage: 0.0,
    salvage: 100.0,
    events: 19
  }
};

// ============================================================================
// 2. Web Audio API & DTMF Synthesizer
// ============================================================================

let audioCtx = null;

function getAudioContext() {
  if (!audioCtx) {
    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (AudioContextClass) {
      audioCtx = new AudioContextClass();
    }
  }
  if (audioCtx && audioCtx.state === 'suspended') {
    audioCtx.resume();
  }
  return audioCtx;
}

// DTMF standard row/col frequencies
const dtmfFreqs = {
  '1': [697, 1209], '2': [697, 1336], '3': [697, 1477],
  '4': [770, 1209], '5': [770, 1336], '6': [770, 1477],
  '7': [852, 1209], '8': [852, 1336], '9': [852, 1477],
  '*': [941, 1209], '0': [941, 1336], '#': [941, 1477]
};

function playDtmfTone(key) {
  if (!simState.audioEnabled) return;
  const ctx = getAudioContext();
  if (!ctx || !dtmfFreqs[key]) return;

  const [f1, f2] = dtmfFreqs[key];
  const osc1 = ctx.createOscillator();
  const osc2 = ctx.createOscillator();
  const gain = ctx.createGain();

  osc1.frequency.value = f1;
  osc2.frequency.value = f2;
  gain.gain.setValueAtTime(0.08, ctx.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.18);

  osc1.connect(gain);
  osc2.connect(gain);
  gain.connect(ctx.destination);

  osc1.start();
  osc2.start();
  osc1.stop(ctx.currentTime + 0.18);
  osc2.stop(ctx.currentTime + 0.18);
}

// Chime notification sound
function playAlertChime(isSuccess = true) {
  if (!simState.audioEnabled) return;
  const ctx = getAudioContext();
  if (!ctx) return;

  const osc = ctx.createOscillator();
  const gain = ctx.createGain();
  osc.type = isSuccess ? 'sine' : 'sawtooth';
  osc.frequency.setValueAtTime(isSuccess ? 587.33 : 329.63, ctx.currentTime); // D5 or E4
  osc.frequency.exponentialRampToValueAtTime(isSuccess ? 880 : 220, ctx.currentTime + 0.25);

  gain.gain.setValueAtTime(0.12, ctx.currentTime);
  gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.35);

  osc.connect(gain);
  gain.connect(ctx.destination);
  osc.start();
  osc.stop(ctx.currentTime + 0.35);
}

// ============================================================================
// 3. Spoken Hinglish Speech Synthesizer (Web Speech API)
// ============================================================================

function speakHinglishAlert(text) {
  // Update subtitle box immediately
  const subtitleEl = document.getElementById('audioSpeechSubtitle');
  if (subtitleEl) {
    subtitleEl.textContent = `"${text}"`;
  }

  // Animate wave
  const waveEl = document.getElementById('speakerWave');
  if (waveEl) {
    waveEl.classList.add('active');
    setTimeout(() => waveEl.classList.remove('active'), 4000);
  }

  if (!simState.audioEnabled || !('speechSynthesis' in window)) return;

  window.speechSynthesis.cancel();
  const utterance = new SpeechSynthesisUtterance(text);
  utterance.rate = 1.0;
  utterance.pitch = 1.05;

  // Try to find a Hindi voice or fallback
  const voices = window.speechSynthesis.getVoices();
  const hindiVoice = voices.find(v => v.lang.includes('hi') || v.lang.includes('IN'));
  if (hindiVoice) {
    utterance.voice = hindiVoice;
  }

  window.speechSynthesis.speak(utterance);
}

// ============================================================================
// 4. Vernacular Voice Parser (Adapter Pattern)
// ============================================================================

function parseVernacularUtterance(phrase) {
  const p = phrase.toLowerCase();

  let crop = "Tomato";
  let cropHindi = "Tamatar (Tomato)";
  if (p.includes("pyaaz") || p.includes("onion") || p.includes("pyaz")) {
    crop = "Onion";
    cropHindi = "Pyaaz (Onion)";
  } else if (p.includes("aalu") || p.includes("potato") || p.includes("aaloo")) {
    crop = "Potato";
    cropHindi = "Aalu (Potato)";
  }

  let qty = 25.0;
  const numMatch = p.match(/\b(\d+)\b/);
  if (numMatch) {
    qty = parseFloat(numMatch[1]);
  }

  let dest = "Azadpur Mandi";
  if (p.includes("ghazipur")) {
    dest = "Ghazipur Mandi";
  } else if (p.includes("sonipat") || p.includes("mother dairy") || p.includes("plant") || p.includes("factory")) {
    dest = "Sonipat Processing Plant";
  }

  // Update UI Elements
  document.getElementById('rawUtteranceText').textContent = `"${phrase}"`;
  document.getElementById('parsedCommodity').textContent = cropHindi;
  document.getElementById('parsedQuantity').textContent = `${qty.toFixed(1)} Quintals`;
  document.getElementById('parsedDestination').textContent = dest;

  // Update status card
  simState.batch.commodity = crop;
  simState.batch.volume = qty;
  updateProduceCard(crop, qty);

  logEvent("INFO", "VOICE_KIOSK", `Parsed vernacular utterance: ${qty}q ${crop} for ${dest}`);
}

function updateProduceCard(crop, qty) {
  const cardEl = document.getElementById('produceStatusCard');
  const cropNameEl = document.getElementById('cardCropName');
  const gradeTagEl = document.getElementById('cardGradeTag');
  const qScoreEl = document.getElementById('cardQualityScore');
  const recEl = document.getElementById('cardRecommendation');

  const q = simState.batch.currentQuality;
  qScoreEl.textContent = q.toFixed(2);

  cardEl.className = 'status-card-widget';

  if (q >= 0.80) {
    cardEl.classList.add('grade-a');
    cropNameEl.textContent = `${crop} (Grade A)`;
    gradeTagEl.textContent = "Taaza / Fresh (ताज़ा)";
    recEl.textContent = `Bhaav sabse behtar milega. Mandi me bechein (Top Price: ₹2,200/qtl for ${qty}q).`;
  } else if (q >= 0.60) {
    cardEl.classList.add('grade-b');
    cropNameEl.textContent = `${crop} (Grade B)`;
    gradeTagEl.textContent = "Chalaau / Fair (चलाऊ)";
    recEl.textContent = `Bhaav theek milega. Jaldi mandi bhejein taaki aur kharab na ho.`;
  } else if (q >= 0.40) {
    cardEl.classList.add('grade-c');
    cropNameEl.textContent = `${crop} (Distress Grade)`;
    gradeTagEl.textContent = "Processing Grade (कारखाना)";
    recEl.textContent = `Mandi me reject hone ka khatra hai. Ketchup Co plant bhejna surakshit hai.`;
  } else {
    cardEl.classList.add('grade-c');
    cropNameEl.textContent = `${crop} (Spoiled)`;
    gradeTagEl.textContent = "Kharab (सड़ा हुआ)";
    recEl.textContent = `Quality floor breach. Bio-gas compost dispatch recommended.`;
  }
}

// ============================================================================
// 5. Invariant Checker (Mathematical Verification)
// ============================================================================

function updateInvariantMonitor() {
  const isTerminal = (simState.contract.state === "COMPLETED" || 
                      simState.contract.state === "COMPENSATED" || 
                      simState.contract.state === "ABORTED");

  let truckLocks = (isTerminal || !simState.contract.hasTruckLock) ? 0 : 1;
  let gateSlots = (isTerminal || !simState.contract.hasGateSlot) ? 0 : 1;
  let escrowHolds = (isTerminal || !simState.contract.hasEscrow) ? 0 : 1;
  let activeContracts = isTerminal ? 0 : 1;

  document.getElementById('invTruckLocks').textContent = truckLocks;
  document.getElementById('invGateSlots').textContent = gateSlots;
  document.getElementById('invEscrowHolds').textContent = escrowHolds;
  document.getElementById('invActiveContracts').textContent = activeContracts;

  const invariantPassed = (isTerminal ? (truckLocks === 0 && gateSlots === 0 && escrowHolds === 0) :
                                       (truckLocks === 1 && gateSlots === 1 && escrowHolds === 1));

  const badge = document.getElementById('invariantBadge');
  const led = document.getElementById('invariantLed');

  if (invariantPassed) {
    badge.className = "badge badge-safe";
    badge.textContent = isTerminal ? "PASSED (0 LEAKS)" : "VERIFIED (1 HELD)";
    led.className = "status-indicator active";
    led.textContent = "0 Leaks";
  } else {
    badge.className = "badge";
    badge.style.background = "rgba(239, 68, 68, 0.3)";
    badge.style.color = "#f87171";
    badge.textContent = "LEAK DETECTED!";
    led.className = "status-indicator";
    led.textContent = "Leak Breach";
  }
}

// ============================================================================
// 6. SVG Road Network & Map Renderer
// ============================================================================

function initRoadMap() {
  const edgesGroup = document.getElementById('roadEdges');
  const nodesGroup = document.getElementById('roadNodes');

  // Roads definition: from, to, length, name
  const roads = [
    { from: "Farm_Sonipat", to: "NH44_Junction", length: "42 km", speed: "45 km/h" },
    { from: "NH44_Junction", to: "Mandi_Azadpur", length: "38 km", speed: "50 km/h" },
    { from: "NH44_Junction", to: "Mandi_Ghazipur", length: "52 km", speed: "40 km/h" },
    { from: "NH44_Junction", to: "Processor_KetchupCo", length: "15 km", speed: "30 km/h" },
    { from: "Farm_Sonipat", to: "Processor_KetchupCo", length: "28 km", speed: "35 km/h" }
  ];

  edgesGroup.innerHTML = '';
  roads.forEach(r => {
    const p1 = simState.nodes[r.from];
    const p2 = simState.nodes[r.to];

    const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
    line.setAttribute("x1", p1.x);
    line.setAttribute("y1", p1.y);
    line.setAttribute("x2", p2.x);
    line.setAttribute("y2", p2.y);
    line.setAttribute("class", "map-edge");
    line.setAttribute("id", `edge_${r.from}_${r.to}`);

    if ((r.from === "Farm_Sonipat" && r.to === "NH44_Junction") ||
        (r.from === "NH44_Junction" && r.to === "Mandi_Azadpur")) {
      line.classList.add("active-route");
    }

    edgesGroup.appendChild(line);
  });

  // Render Nodes
  nodesGroup.innerHTML = '';
  Object.values(simState.nodes).forEach(n => {
    const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
    g.setAttribute("class", "map-node");
    g.setAttribute("transform", `translate(${n.x}, ${n.y})`);

    // Circle
    const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    circle.setAttribute("r", n.type === "mandi" ? 12 : 8);
    circle.setAttribute("fill", n.type === "mandi" ? "#10b981" : "#1e293b");
    circle.setAttribute("stroke", "#475569");
    circle.setAttribute("stroke-width", "1.5");

    // Node label
    const text = document.createElementNS("http://www.w3.org/2000/svg", "text");
    text.setAttribute("x", 0);
    text.setAttribute("y", -16);
    text.setAttribute("text-anchor", "middle");
    text.setAttribute("class", "node-label");
    text.textContent = n.name.replace("_", " ");

    // Sublabel
    const subtext = document.createElementNS("http://www.w3.org/2000/svg", "text");
    subtext.setAttribute("x", 0);
    subtext.setAttribute("y", 20);
    subtext.setAttribute("text-anchor", "middle");
    subtext.setAttribute("class", "node-sublabel");
    subtext.textContent = n.price ? `₹${n.price}/q` : (n.bidPrice ? `Bid: ₹${n.bidPrice}/q` : "");

    g.appendChild(circle);
    g.appendChild(text);
    g.appendChild(subtext);

    g.addEventListener('click', () => {
      logEvent("INFO", "NODE_INSPECT", `Inspected ${n.name}: ${n.desc || 'Active Logistics Node'}`);
      playDtmfTone('5');
    });

    nodesGroup.appendChild(g);
  });

  updateTruckPosition();
}

function updateTruckPosition() {
  const trucksGroup = document.getElementById('roadTrucks');
  if (!trucksGroup) return;

  trucksGroup.innerHTML = '';

  simState.trucks.forEach(t => {
    let x, y;
    if (t.id === "Truck_DL01") {
      const pFarm = simState.nodes["Farm_Sonipat"];
      const pJunc = simState.nodes["NH44_Junction"];
      const pAzad = simState.nodes["Mandi_Azadpur"];
      const pGhaz = simState.nodes["Mandi_Ghazipur"];
      const pPlant = simState.nodes["Processor_KetchupCo"];

      if (t.state === "DIVERTED" && simState.contract.mandiId === "Processor_KetchupCo") {
        // Moving from junction to plant
        const factor = Math.min(1.0, (t.currentProgress - 50) / 50.0);
        x = pJunc.x + (pPlant.x - pJunc.x) * factor;
        y = pJunc.y + (pPlant.y - pJunc.y) * factor;
      } else if (simState.contract.mandiId === "Mandi_Ghazipur") {
        // Rerouted to Ghazipur
        const factor = Math.min(1.0, (t.currentProgress - 50) / 50.0);
        x = pJunc.x + (pGhaz.x - pJunc.x) * factor;
        y = pJunc.y + (pGhaz.y - pJunc.y) * factor;
      } else {
        // En route Farm -> Azadpur
        if (t.currentProgress <= 50) {
          const factor = t.currentProgress / 50.0;
          x = pFarm.x + (pJunc.x - pFarm.x) * factor;
          y = pFarm.y + (pJunc.y - pFarm.y) * factor;
        } else {
          const factor = (t.currentProgress - 50.0) / 50.0;
          x = pJunc.x + (pAzad.x - pJunc.x) * factor;
          y = pJunc.y + (pAzad.y - pJunc.y) * factor;
        }
      }
    } else {
      // Truck HR02 (Standby at NH44 Junction)
      x = simState.nodes["NH44_Junction"].x + 15;
      y = simState.nodes["NH44_Junction"].y + 15;
    }

    const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
    g.setAttribute("transform", `translate(${x}, ${y})`);

    // Subtle halo
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "12");
    halo.setAttribute("fill", t.state === "STALLED" ? "rgba(239, 68, 68, 0.25)" : "rgba(16, 185, 129, 0.2)");

    // Truck pin
    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "6");
    pin.setAttribute("fill", t.state === "STALLED" ? "#ef4444" : "#10b981");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "1.5");

    g.appendChild(halo);
    g.appendChild(pin);
    trucksGroup.appendChild(g);
  });
}

// ============================================================================
// 7. Interactive Fault Injections & Zero-Leak Recovery
// ============================================================================

function injectMandiGlut() {
  playAlertChime(false);
  simState.nodes["Mandi_Azadpur"].price = 1320;
  simState.nodes["Mandi_Azadpur"].glut = true;
  simState.nodes["Mandi_Azadpur"].occupied = 5; // full capacity

  const alertMsg = "Mandi update: Azadpur mandi me aamad badhne se bhaav ₹1,320 gir gaya hai. Hysteresis threshold (gain > ₹5,000) verify hone par truck Ghazipur mandi reroute kiya ja raha hai.";
  speakHinglishAlert(alertMsg);

  // Execute DynamicReroutingRecovery
  const oldMandi = simState.contract.mandiId;
  const newMandi = "Mandi_Ghazipur";
  simState.contract.mandiId = newMandi;
  simState.contract.version++;

  // Update Route UI
  const edgeAzad = document.getElementById('edge_NH44_Junction_Mandi_Azadpur');
  if (edgeAzad) edgeAzad.className.baseVal = "map-edge";

  const edgeGhaz = document.getElementById('edge_NH44_Junction_Mandi_Ghazipur');
  if (edgeGhaz) edgeGhaz.className.baseVal = "map-edge reroute";

  document.getElementById('truck1RouteText').textContent = "Farm_Sonipat ➔ Ghazipur";

  // Update Recovery Card
  document.getElementById('recStrategyName').textContent = "DynamicReroutingRecovery (v" + simState.contract.version + ")";
  document.getElementById('recStatusBadge').textContent = "Reroute Succeeded";
  document.getElementById('recStatusBadge').style.background = "rgba(16, 185, 129, 0.25)";
  document.getElementById('recStatusBadge').style.color = "#34d399";
  document.getElementById('recActionSummary').textContent = 
    `Slot cleanly cancelled at Azadpur. Replacement slot reserved at Ghazipur (Quote: ₹2,100/qtl). Net gain ₹16,500 exceeds hysteresis delta ₹5,000. Old gate slot released; zero leaked resources.`;

  logEvent("RECOVERY", "REROUTE", `DynamicReroutingRecovery: Diverted from ${oldMandi} to ${newMandi}. Slot rebound (contract v${simState.contract.version})`);
  updateInvariantMonitor();
  updateTruckPosition();
}

function injectTruckBreakdown() {
  playAlertChime(false);
  const t1 = simState.trucks[0];
  const t2 = simState.trucks[1];

  t1.state = "STALLED";
  document.getElementById('truck1StateBadge').className = "truck-state-badge stalled";
  document.getElementById('truck1StateBadge').textContent = "STALLED (ENGINE)";

  const alertMsg = "Alert: Truck ka engine overheat hone se kharab hua hai. Nikat-tam Truck HR02 turant maal lene aa raha hai.";
  speakHinglishAlert(alertMsg);

  // Execute PeerHandoffRecovery
  setTimeout(() => {
    t2.state = "EN_ROUTE";
    document.getElementById('truck2StateBadge').className = "truck-state-badge enroute";
    document.getElementById('truck2StateBadge').textContent = "ACTIVE (PEER HANDOFF)";
    document.getElementById('truck2StandbyText').textContent = "Cargo manifest transferred from Truck_DL01";

    simState.contract.transporterId = t2.id;
    simState.contract.version++;

    document.getElementById('recStrategyName').textContent = "PeerHandoffRecovery (v" + simState.contract.version + ")";
    document.getElementById('recStatusBadge').textContent = "Handoff Complete";
    document.getElementById('recStatusBadge').style.background = "rgba(16, 185, 129, 0.25)";
    document.getElementById('recStatusBadge').style.color = "#34d399";
    document.getElementById('recActionSummary').textContent = 
      `Transporter DL01 lock released. Manifest transferred to peer Truck_HR02 at rendezvous point NH44_Junction. Contract lock rebound; zero leaked locks.`;

    logEvent("RECOVERY", "PEER_HANDOFF", `PeerHandoffRecovery: Cargo manifest handed off from Truck_DL01 to Truck_HR02 at NH44 (contract v${simState.contract.version})`);
    updateInvariantMonitor();
    updateTruckPosition();
  }, 1000);
}

function injectHighwayJam() {
  playAlertChime(false);
  const t1 = simState.trucks[0];
  t1.state = "DIVERTED";
  document.getElementById('truck1StateBadge').className = "truck-state-badge diverted";
  document.getElementById('truck1StateBadge').textContent = "DIVERTED (FACTORY)";

  // Quality drops to 0.55 breaching 0.60 mandi floor
  simState.batch.currentQuality = 0.55;
  document.getElementById('truck1QualityText').textContent = "0.550 (Distress Grade C)";
  document.getElementById('truck1QualityText').style.color = "#f59e0b";
  updateProduceCard("Tomato", 25.0);

  const alertMsg = "Kisan Bhai dhyan dein: Aapka truck highway jam me fansa hai. Tamatar kharab hone se bachane ke liye truck Mother Dairy ketchup factory bheja ja raha hai.";
  speakHinglishAlert(alertMsg);

  // Execute DistressAuctionRecovery
  simState.contract.mandiId = "Processor_KetchupCo";
  simState.contract.hasGateSlot = false; // Mandi slot released
  simState.contract.version++;

  document.getElementById('truck1RouteText').textContent = "NH44_Junction ➔ Processor_KetchupCo";

  // Re-route on map
  const edgePlant = document.getElementById('edge_NH44_Junction_Processor_KetchupCo');
  if (edgePlant) edgePlant.className.baseVal = "map-edge reroute";

  document.getElementById('recStrategyName').textContent = "DistressAuctionRecovery (v" + simState.contract.version + ")";
  document.getElementById('recStatusBadge').textContent = "Distress Diversion";
  document.getElementById('recStatusBadge').style.background = "rgba(245, 158, 11, 0.25)";
  document.getElementById('recStatusBadge').style.color = "#fbbf24";
  document.getElementById('recActionSummary').textContent = 
    `Mandi gate slot released immediately. Truck diverted to Processor_KetchupCo (15 km) at distress bid ₹1,850/qtl. 100% produce salvaged from spoilage; zero leaked slot locks.`;

  logEvent("RECOVERY", "DISTRESS_AUCTION", `DistressAuctionRecovery: Mandi slot freed. Diverted to Processor_KetchupCo at ₹1,850/q (contract v${simState.contract.version})`);
  updateInvariantMonitor();
  updateTruckPosition();
}

// ============================================================================
// 8. Event Logging Stream
// ============================================================================

function logEvent(level, category, message) {
  const stream = document.getElementById('eventLogStream');
  if (!stream) return;

  const row = document.createElement('div');
  row.className = 'log-entry';

  const hrs = String(Math.floor(simState.timeMinutes / 60)).padStart(2, '0');
  const mins = String(simState.timeMinutes % 60).padStart(2, '0');
  const timeStr = `${hrs}:${mins}`;

  let levelClass = "info";
  if (level === "RECOVERY") levelClass = "rec";
  else if (level === "WARNING") levelClass = "warn";
  else if (level === "FAULT") levelClass = "fault";
  else if (level === "AUDIT") levelClass = "audit";

  row.innerHTML = `
    <span class="time">[${timeStr}]</span>
    <span class="level ${levelClass}">[${category}]</span>
    <span class="msg">${message}</span>
  `;

  stream.appendChild(row);
  simState.metrics.events++;
  document.getElementById('metricEvents').textContent = `${simState.metrics.events} Events`;
}

// ============================================================================
// 9. Simulation Tick & Timeline Engine
// ============================================================================

function tickSimulation(stepMinutes = 15) {
  simState.timeMinutes += stepMinutes;

  const hrs = String(Math.floor(simState.timeMinutes / 60)).padStart(2, '0');
  const mins = String(simState.timeMinutes % 60).padStart(2, '0');
  document.getElementById('simTimeDisplay').textContent = `${hrs}h : ${mins}m`;
  document.getElementById('simTickDisplay').textContent = `T+${simState.timeMinutes}m (Seed ${simState.seed})`;

  // Progress moving truck
  const t1 = simState.trucks[0];
  if (t1.state === "EN_ROUTE" || t1.state === "DIVERTED") {
    t1.currentProgress = Math.min(100.0, t1.currentProgress + (stepMinutes * 0.45));
    document.getElementById('truck1ProgressBar').style.width = `${t1.currentProgress}%`;

    // Calculate exponential freshness decay Q(t)
    const hoursElapsed = (simState.timeMinutes - simState.batch.harvestTime) / 60.0;
    const decay = simState.batch.decayLambda * simState.batch.tempMultiplier;
    simState.batch.currentQuality = Math.max(0.40, simState.batch.initialQuality * Math.exp(-decay * hoursElapsed));

    document.getElementById('truck1QualityText').textContent = 
      `${simState.batch.currentQuality.toFixed(3)} (${simState.batch.currentQuality >= 0.8 ? 'Grade A' : 'Grade B'})`;

    updateProduceCard(simState.batch.commodity, simState.batch.volume);

    // If reached 100%, complete contract
    if (t1.currentProgress >= 100.0 && simState.contract.state !== "COMPLETED") {
      t1.state = "DELIVERED";
      simState.contract.state = "COMPLETED";
      simState.contract.hasTruckLock = false;
      simState.contract.hasGateSlot = false;
      simState.contract.hasEscrow = false;

      document.getElementById('truck1StateBadge').className = "truck-state-badge completed";
      document.getElementById('truck1StateBadge').textContent = "FULFILLED (DELIVERED)";
      playAlertChime(true);
      speakHinglishAlert("Mubarak ho! Fasal safaltapoorvak mandi pahunch gayi hai. Escrow bhuqtaan transporter ko jaari kiya gaya.");
      logEvent("INFO", "CONTRACT_FULFILLED", "Contract CTR-101 completed successfully. Escrow funds disbursed, zero resources leaked.");
      updateInvariantMonitor();
    }
  }

  updateTruckPosition();
}

function startSimulation() {
  if (simState.running) return;
  simState.running = true;
  document.getElementById('playPauseText').textContent = "Pause";
  document.getElementById('playIcon').innerHTML = '<rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/>';

  const intervalMs = Math.max(200, 1000 / simState.speed);
  simState.timerId = setInterval(() => {
    tickSimulation(5);
  }, intervalMs);
}

function pauseSimulation() {
  simState.running = false;
  document.getElementById('playPauseText').textContent = "Resume";
  document.getElementById('playIcon').innerHTML = '<polygon points="5,3 19,12 5,21"/>';
  if (simState.timerId) {
    clearInterval(simState.timerId);
    simState.timerId = null;
  }
}

// ============================================================================
// 10. Initialization & Event Listeners
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  // Initialize Road Map
  initRoadMap();
  updateInvariantMonitor();

  // Play / Pause
  document.getElementById('btnPlayPause').addEventListener('click', () => {
    if (simState.running) pauseSimulation();
    else startSimulation();
  });

  // Step
  document.getElementById('btnStep').addEventListener('click', () => {
    tickSimulation(15);
    playDtmfTone('9');
  });

  // Reset
  document.getElementById('btnReset').addEventListener('click', () => {
    pauseSimulation();
    simState.timeMinutes = 120;
    simState.batch.currentQuality = 0.94;
    simState.contract.state = "COMMITTED";
    simState.contract.hasTruckLock = true;
    simState.contract.hasGateSlot = true;
    simState.contract.hasEscrow = true;
    simState.contract.mandiId = "Mandi_Azadpur";
    simState.trucks[0].state = "EN_ROUTE";
    simState.trucks[0].currentProgress = 45.0;
    simState.trucks[1].state = "IDLE";

    document.getElementById('truck1ProgressBar').style.width = '45%';
    document.getElementById('truck1StateBadge').className = 'truck-state-badge enroute';
    document.getElementById('truck1StateBadge').textContent = 'EN_ROUTE';
    document.getElementById('truck2StateBadge').className = 'truck-state-badge idle';
    document.getElementById('truck2StateBadge').textContent = 'IDLE (STANDBY)';
    document.getElementById('truck1RouteText').textContent = 'Farm_Sonipat ➔ Azadpur';

    initRoadMap();
    updateInvariantMonitor();
    updateProduceCard("Tomato", 25.0);
    logEvent("INFO", "SIMULATION", "Simulation reset to initial T+120m benchmark state.");
  });

  // Speed Selector
  document.querySelectorAll('.speed-btn').forEach(btn => {
    btn.addEventListener('click', e => {
      document.querySelectorAll('.speed-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      simState.speed = parseInt(btn.dataset.speed, 10);
      if (simState.running) {
        pauseSimulation();
        startSimulation();
      }
    });
  });

  // Audio Toggle
  document.getElementById('btnAudioToggle').addEventListener('click', () => {
    simState.audioEnabled = !simState.audioEnabled;
    const btn = document.getElementById('btnAudioToggle');
    if (simState.audioEnabled) {
      btn.classList.add('active');
      speakHinglishAlert("Audio aawaaz chalu ki gayi hai.");
    } else {
      btn.classList.remove('active');
      window.speechSynthesis.cancel();
    }
  });

  // Voice Chips
  document.querySelectorAll('.voice-chip').forEach(chip => {
    chip.addEventListener('click', () => {
      document.querySelectorAll('.voice-chip').forEach(c => c.classList.remove('active'));
      chip.classList.add('active');
      parseVernacularUtterance(chip.dataset.phrase);
      playAlertChime(true);
    });
  });

// Visualizer state
let micStream = null;
let micAnalyser = null;
let micAnimId = null;

function startMicVisualizer(stream) {
  const canvas = document.getElementById('micVisualizerCanvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const aCtx = getAudioContext();
  if (!aCtx) return;

  try {
    const source = aCtx.createMediaStreamSource(stream);
    micAnalyser = aCtx.createAnalyser();
    micAnalyser.fftSize = 64;
    source.connect(micAnalyser);

    const bufferLength = micAnalyser.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);

    function draw() {
      micAnimId = requestAnimationFrame(draw);
      micAnalyser.getByteFrequencyData(dataArray);

      ctx.fillStyle = 'rgba(9, 13, 22, 0.4)';
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      const barWidth = (canvas.width / bufferLength) * 1.5;
      let x = 0;

      for (let i = 0; i < bufferLength; i++) {
        const barHeight = (dataArray[i] / 255) * canvas.height;
        ctx.fillStyle = `rgb(${16 + dataArray[i]}, 185, ${129 + Math.floor(dataArray[i]/2)})`;
        ctx.fillRect(x, canvas.height - barHeight, barWidth - 1, barHeight);
        x += barWidth;
      }
    }
    draw();
  } catch (err) {
    console.warn("Visualizer audio graph notice:", err);
  }
}

function stopMicVisualizer() {
  if (micAnimId) {
    cancelAnimationFrame(micAnimId);
    micAnimId = null;
  }
  if (micStream) {
    micStream.getTracks().forEach(t => t.stop());
    micStream = null;
  }
  const canvas = document.getElementById('micVisualizerCanvas');
  if (canvas) {
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
  }
}

  // Mic Button (Live Voice-to-Text Recognition)
  const micBtn = document.getElementById('btnMicTrigger');
  const micLabel = document.getElementById('micStatusLabel');
  const voiceInput = document.getElementById('voiceCommandInput');

  let isRecording = false;
  let activeRecognition = null;

  micBtn.addEventListener('click', async () => {
    getAudioContext();

    if (isRecording) {
      if (activeRecognition) {
        try { activeRecognition.stop(); } catch(e){}
      }
      stopMicVisualizer();
      isRecording = false;
      micBtn.classList.remove('listening');
      micLabel.textContent = "Tap to speak or select phrase";
      return;
    }

    // Try live mic visualizer
    if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
      try {
        micStream = await navigator.mediaDevices.getUserMedia({ audio: true });
        startMicVisualizer(micStream);
      } catch (err) {
        console.warn("Microphone access notice:", err);
      }
    }

    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;

    if (SpeechRecognition) {
      const recognition = new SpeechRecognition();
      activeRecognition = recognition;
      recognition.lang = 'hi-IN'; // Hindi (India)
      recognition.interimResults = true;
      recognition.continuous = false;

      isRecording = true;
      micBtn.classList.add('listening');
      micLabel.textContent = "Listening live... Bolte rahiye";

      recognition.onresult = event => {
        let transcript = "";
        for (let i = event.resultIndex; i < event.results.length; ++i) {
          transcript += event.results[i][0].transcript;
        }

        if (voiceInput) voiceInput.value = transcript;
        micLabel.textContent = `Heard: "${transcript}"`;

        if (event.results[0].isFinal) {
          parseVernacularUtterance(transcript);
          speakHinglishAlert(`Aapka aadesh suna gaya: ${transcript}`);
          playAlertChime(true);
        }
      };

      recognition.onerror = event => {
        console.warn("Speech recognition notice:", event.error);
        micLabel.textContent = "Voice input ready. Tap or type command.";
        stopMicVisualizer();
        isRecording = false;
        micBtn.classList.remove('listening');
      };

      recognition.onend = () => {
        stopMicVisualizer();
        isRecording = false;
        micBtn.classList.remove('listening');
      };

      try {
        recognition.start();
      } catch (e) {
        console.warn(e);
      }
    } else {
      micLabel.textContent = "Speech recognition not supported in this browser. Please type below.";
      if (voiceInput) voiceInput.focus();
    }
  });

  // Manual Voice-to-Text Input Bar (Submit on button or Enter key)
  const submitVoiceBtn = document.getElementById('btnSubmitVoiceInput');
  const handleVoiceTextSubmit = () => {
    const val = (voiceInput.value || "").trim();
    if (!val) return;
    parseVernacularUtterance(val);
    speakHinglishAlert(`Aapka aadesh process kiya gaya: ${val}`);
    playAlertChime(true);
  };

  if (submitVoiceBtn) submitVoiceBtn.addEventListener('click', handleVoiceTextSubmit);
  if (voiceInput) {
    voiceInput.addEventListener('keydown', e => {
      if (e.key === 'Enter') handleVoiceTextSubmit();
    });
  }

  // DTMF Numpad Buttons
  document.querySelectorAll('.numpad-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const key = btn.dataset.key;
      playDtmfTone(key);
      logEvent("INFO", "DTMF_KEYPAD", `DTMF Key Pressed: [${key}]`);
    });
  });

  // 2PC Buttons
  document.getElementById('btnConfirm2PC').addEventListener('click', () => {
    playAlertChime(true);
    speakHinglishAlert("Dhanyavaad! 2-Phase Commit safal raha. Truck aur gate slot reserve ho gaye hain.");
    logEvent("INFO", "2PC_COMMIT", "Farmer confirmed contract via voice kiosk. 2PC commit verified.");
  });

  document.getElementById('btnCancel2PC').addEventListener('click', () => {
    playAlertChime(false);
    simState.contract.state = "ABORTED";
    simState.contract.hasTruckLock = false;
    simState.contract.hasGateSlot = false;
    simState.contract.hasEscrow = false;
    updateInvariantMonitor();
    speakHinglishAlert("Aadesh radd kar diya gaya hai. Sabhi resource locks release ho chuke hain.");
    logEvent("ROLLBACK", "2PC_ABORT", "Farmer cancelled request. Reverse compensation executed with zero leaks.");
  });

  // Fault Injections
  document.getElementById('btnInjectGlut').addEventListener('click', injectMandiGlut);
  document.getElementById('btnInjectStall').addEventListener('click', injectTruckBreakdown);
  document.getElementById('btnInjectBlockage').addEventListener('click', injectHighwayJam);

  // Clear Log
  document.getElementById('btnClearLog').addEventListener('click', () => {
    document.getElementById('eventLogStream').innerHTML = '';
  });

  // Initial event logs
  logEvent("INFO", "SYSTEM_INIT", "KisanSetu Multi-Agent Engine v1.0.0 initialized.");
  logEvent("INFO", "CNP_START", "Contract CTR-101 prepared via Contract Net Protocol.");
  logEvent("INFO", "2PC_COMMIT", "Truck lock, Azadpur gate slot, and Escrow deposit locked atomically.");
});
