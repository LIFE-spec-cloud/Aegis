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
  audioEnabled: false, // Text-to-speech and audio alerts disabled
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

  // 2-Phase Commit Contract 1 (Primary Batch)
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

  // Crop Batch 2 (Concurrent Harvest: Farmer Suresh)
  batch2: {
    id: "BATCH-DEMO-02",
    commodity: "Onion",
    volume: 40.0,
    initialQuality: 0.96,
    currentQuality: 0.95,
    decayLambda: 0.015,
    tempMultiplier: 1.10,
    harvestTime: 0
  },

  // 2-Phase Commit Contract 2 (Concurrent Shipment)
  contract2: {
    id: "CTR-102",
    state: "IDLE", // IDLE, PREPARING, COMMITTED, COMPLETED, ABORTED
    version: 1,
    farmerId: "Farmer_Suresh",
    transporterId: "Truck_HR02",
    mandiId: "Mandi_Ghazipur",
    slotTime: 210,
    freightCost: 1800,
    depositAmount: 400,
    hasTruckLock: false,
    hasGateSlot: false,
    hasEscrow: false
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
// 3. System Announcements & Notification Feed (Silent Visual Updates)
// ============================================================================

// Immediately cancel and suppress browser TTS
if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
  try {
    window.speechSynthesis.cancel();
    window.speechSynthesis.speak = function() {}; // Prevent any speech output
  } catch (e) {}
}

function speakHinglishAlert(text) {
  // Ensure audio speech is completely silenced
  if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
    try { window.speechSynthesis.cancel(); } catch(e) {}
  }
  // Update announcement display quietly without audio speech
  const subtitleEl = document.getElementById('audioSpeechSubtitle');
  if (subtitleEl) {
    subtitleEl.textContent = `"${text}"`;
  }
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
  const isTerminal1 = (simState.contract.state === "COMPLETED" || 
                       simState.contract.state === "COMPENSATED" || 
                       simState.contract.state === "ABORTED");

  const isTerminal2 = (simState.contract2.state === "IDLE" ||
                       simState.contract2.state === "COMPLETED" || 
                       simState.contract2.state === "COMPENSATED" || 
                       simState.contract2.state === "ABORTED");

  let truckLocks = 0;
  let gateSlots = 0;
  let escrowHolds = 0;
  let activeContracts = 0;

  if (!isTerminal1) {
    activeContracts++;
    if (simState.contract.hasTruckLock) truckLocks++;
    if (simState.contract.hasGateSlot) gateSlots++;
    if (simState.contract.hasEscrow) escrowHolds++;
  }

  if (simState.contract2.state === "COMMITTED" || simState.contract2.state === "PREPARING") {
    activeContracts++;
    if (simState.contract2.hasTruckLock) truckLocks++;
    if (simState.contract2.hasGateSlot) gateSlots++;
    if (simState.contract2.hasEscrow) escrowHolds++;
  }

  document.getElementById('invTruckLocks').textContent = truckLocks;
  document.getElementById('invGateSlots').textContent = gateSlots;
  document.getElementById('invEscrowHolds').textContent = escrowHolds;
  document.getElementById('invActiveContracts').textContent = activeContracts;

  const totalLocks = truckLocks + gateSlots + escrowHolds;
  const invariantPassed = (totalLocks === activeContracts * 3);

  const badge = document.getElementById('invariantBadge');
  const led = document.getElementById('invariantLed');

  if (invariantPassed) {
    badge.className = "badge badge-safe";
    badge.textContent = activeContracts === 0 ? "PASSED (0 LEAKS)" : `VERIFIED (${activeContracts} BATCH ACTIVE)`;
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
// 6. Interactive Node Inspector (Option 3)
// ============================================================================

function openNodeInspector(nodeKey) {
  const n = simState.nodes[nodeKey];
  if (!n) return;

  const inspector = document.getElementById('nodeInspectorCard');
  if (!inspector) return;

  const typeEl = document.getElementById('inspectNodeType');
  const titleEl = document.getElementById('inspectNodeTitle');
  const descEl = document.getElementById('inspectNodeDesc');
  const priceEl = document.getElementById('inspectNodePrice');
  const occEl = document.getElementById('inspectNodeOccupancy');
  const winEl = document.getElementById('inspectNodeWindow');
  const inbEl = document.getElementById('inspectNodeInbound');

  titleEl.textContent = n.name.replace('_', ' ');

  if (n.type === "mandi") {
    typeEl.textContent = "APMC WHOLESALE MANDI";
    typeEl.className = "badge-node-type";
    descEl.textContent = nodeKey === "Mandi_Azadpur" 
      ? "Asia's largest primary APMC wholesale fruits & vegetables terminal yard." 
      : "Major trans-Yamuna agricultural marketing yard serving East Delhi-NCR.";
    priceEl.textContent = n.glut ? `₹${n.price}/q (GLUT CRASH)` : `₹${n.price}/qtl`;
    priceEl.style.color = n.glut ? "#ef4444" : "#10b981";
    occEl.textContent = `${n.occupied} / ${n.capacity} Docks (${n.glut ? '100% Clogged' : 'Normal Flow'})`;
    winEl.textContent = "04:00 - 08:30 AM (Peak Open Outcry Auction)";
  } else if (n.type === "farm") {
    typeEl.textContent = "HARVEST ORIGIN FARM";
    typeEl.className = "badge-node-type";
    descEl.textContent = "Farmer Ramesh & Suresh cooperative holding. Sonipat Perishable Cluster.";
    priceEl.textContent = "Cost: ₹1,200/qtl";
    priceEl.style.color = "#38bdf8";
    occEl.textContent = "2 Dispatch Docks (Active Loading)";
    winEl.textContent = "05:00 AM Harvest Cycle";
  } else if (n.type === "junction") {
    typeEl.textContent = "TRANSIT & RENDEZVOUS HUB";
    typeEl.className = "badge-node-type";
    descEl.textContent = "Critical corridor artery connecting Haryana harvest belt with Delhi APMC network.";
    priceEl.textContent = "Toll: ₹180 / truck";
    priceEl.style.color = "#94a3b8";
    occEl.textContent = "Highway Speed: 45 km/h";
    winEl.textContent = "24x7 Continuous Corridor";
  } else if (n.type === "plant") {
    typeEl.textContent = "AGRO-PROCESSING PLANT";
    typeEl.className = "badge-node-type";
    descEl.textContent = "Mother Dairy commercial processing unit (Puree, Paste, Ketchup Manufacturing).";
    priceEl.textContent = `Bid: ₹${n.bidPrice}/qtl (Distress Salvage)`;
    priceEl.style.color = "#f59e0b";
    occEl.textContent = "Industrial Intake: 1,500 qtl/day";
    winEl.textContent = "Instant Distress Intake Queue";
  }

  // Active inbound trucks targeting this node
  const inbounds = [];
  if (simState.contract.mandiId === nodeKey && (simState.trucks[0].state === "EN_ROUTE" || simState.trucks[0].state === "DIVERTED")) {
    inbounds.push(`Truck_DL01 (${simState.trucks[0].currentProgress.toFixed(0)}%)`);
  }
  if (simState.contract2.mandiId === nodeKey && simState.trucks[1].state === "EN_ROUTE") {
    inbounds.push(`Truck_HR02 (${simState.trucks[1].currentProgress.toFixed(0)}%)`);
  }
  if (simState.contract.transporterId === "Truck_HR02" && simState.contract.mandiId === nodeKey && simState.trucks[1].state === "EN_ROUTE") {
    inbounds.push(`Truck_HR02 (${simState.trucks[1].currentProgress.toFixed(0)}%)`);
  }

  inbEl.textContent = inbounds.length > 0 ? inbounds.join(", ") : "No incoming vehicles";

  inspector.classList.remove('hidden');
  logEvent("INFO", "NODE_INSPECT", `Inspected node [${n.name}]: Live status retrieved.`);
}

// ============================================================================
// 7. SVG Road Network & Map Renderer
// ============================================================================

function updateRouteHighlights() {
  const roads = [
    { from: "Farm_Sonipat", to: "NH44_Junction" },
    { from: "NH44_Junction", to: "Mandi_Azadpur" },
    { from: "NH44_Junction", to: "Mandi_Ghazipur" },
    { from: "NH44_Junction", to: "Processor_KetchupCo" },
    { from: "Farm_Sonipat", to: "Processor_KetchupCo" }
  ];

  const dest1 = simState.contract.mandiId;
  const isB2 = (simState.contract2.state === "COMMITTED" || simState.contract2.state === "PREPARING");
  const dest2 = simState.contract2.mandiId;

  roads.forEach(r => {
    const edge = document.getElementById(`edge_${r.from}_${r.to}`);
    if (!edge) return;

    let isActive = false;
    let isReroute = false;

    // Origin to NH-44 corridor
    if (r.from === "Farm_Sonipat" && r.to === "NH44_Junction") {
      isActive = true;
    }

    // NH-44 corridor to Primary Contract Destination
    if (r.from === "NH44_Junction" && r.to === dest1) {
      isActive = true;
      if (dest1 === "Mandi_Ghazipur" || dest1 === "Processor_KetchupCo") {
        isReroute = true;
      }
    }

    // NH-44 corridor to Batch 2 Destination (if active)
    if (isB2 && r.from === "NH44_Junction" && r.to === dest2) {
      isActive = true;
    }

    if (isActive) {
      edge.className.baseVal = isReroute ? "map-edge active-route reroute" : "map-edge active-route";
    } else {
      edge.className.baseVal = "map-edge";
    }
  });
}

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

    edgesGroup.appendChild(line);
  });

  updateRouteHighlights();

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
      openNodeInspector(n.name);
    });

    nodesGroup.appendChild(g);
  });

  updateTruckPosition();
}

function updateTruckPosition() {
  const trucksGroup = document.getElementById('roadTrucks');
  if (!trucksGroup) return;

  trucksGroup.innerHTML = '';

  const t1 = simState.trucks[0];
  const t2 = simState.trucks[1];

  const pFarm = simState.nodes["Farm_Sonipat"];
  const pJunc = simState.nodes["NH44_Junction"];
  const pAzad = simState.nodes["Mandi_Azadpur"];
  const pGhaz = simState.nodes["Mandi_Ghazipur"];
  const pPlant = simState.nodes["Processor_KetchupCo"];

  // 1. Calculate Truck DL01 Coordinates
  let x1, y1;
  const pDest1 = t1.state === "DIVERTED" ? pPlant : (simState.contract.mandiId === "Mandi_Ghazipur" ? pGhaz : (simState.contract.mandiId === "Processor_KetchupCo" ? pPlant : pAzad));
  if (t1.state === "STALLED" || simState.contract.transporterId === "Truck_HR02") {
    // If DL01 is stalled or if HR02 took over the contract, DL01 stays parked at NH-44 breakdown pad
    x1 = pJunc.x - 22;
    y1 = pJunc.y - 12;
    t1.state = "STALLED";
  } else if (t1.state === "DELIVERED") {
    // Dock Bay 1: Parked on West unloading bay of destination node
    x1 = pDest1.x - 48;
    y1 = pDest1.y - 28;
  } else if (t1.currentProgress <= 50.0) {
    const factor = Math.max(0.0, Math.min(1.0, t1.currentProgress / 50.0));
    x1 = pFarm.x + (pJunc.x - pFarm.x) * factor;
    y1 = pFarm.y + (pJunc.y - pFarm.y) * factor - 4; // Lane 1 offset
  } else {
    const factor = Math.max(0.0, Math.min(1.0, (t1.currentProgress - 50.0) / 50.0));
    x1 = pJunc.x + (pDest1.x - pJunc.x) * factor;
    y1 = pJunc.y + (pDest1.y - pJunc.y) * factor - 4;
  }

  // 2. Calculate Truck HR02 Coordinates
  let x2, y2;
  const isBatch2 = (simState.contract2.state === "COMMITTED" && simState.contract2.transporterId === "Truck_HR02");
  const pDest2 = simState.nodes[simState.contract2.mandiId] || simState.nodes["Mandi_Ghazipur"];
  const pDestPeer = t2.state === "DIVERTED" ? pPlant : (simState.contract.mandiId === "Mandi_Ghazipur" ? pGhaz : (simState.contract.mandiId === "Processor_KetchupCo" ? pPlant : pAzad));
  const pDestFinal2 = isBatch2 ? pDest2 : pDestPeer;

  if (t2.state === "IDLE") {
    // Parked beside NH44 junction standby pad
    x2 = pJunc.x + 24;
    y2 = pJunc.y + 24;
  } else if (t2.state === "DELIVERED") {
    // Dock Bay 2: Parked on East unloading bay of destination node
    x2 = pDestFinal2.x + 48;
    y2 = pDestFinal2.y - 28;
  } else if (isBatch2) {
    // Multi-Harvest Batch 2 en route from Farm_Sonipat to Ghazipur
    if (t2.currentProgress <= 50.0) {
      const factor = Math.max(0.0, Math.min(1.0, t2.currentProgress / 50.0));
      x2 = pFarm.x + (pJunc.x - pFarm.x) * factor + 4;
      y2 = pFarm.y + (pJunc.y - pFarm.y) * factor + 6; // Lane 2 offset
    } else {
      const factor = Math.max(0.0, Math.min(1.0, (t2.currentProgress - 50.0) / 50.0));
      x2 = pJunc.x + (pDest2.x - pJunc.x) * factor;
      y2 = pJunc.y + (pDest2.y - pJunc.y) * factor + 4;
    }
  } else {
    // In active transit from NH44 Junction to destination (Peer Takeover / Divert)
    const factor = Math.max(0.0, Math.min(1.0, (t2.currentProgress - 50.0) / 50.0));
    x2 = pJunc.x + (pDestFinal2.x - pJunc.x) * factor;
    y2 = pJunc.y + (pDestFinal2.y - pJunc.y) * factor + 4;
  }

  // 3. Draw Cargo Handoff Transfer Beam if breakdown occurred
  if (t1.state === "STALLED" && (t2.state === "EN_ROUTE" || t2.state === "DIVERTED" || t2.state === "DELIVERED")) {
    const beam = document.createElementNS("http://www.w3.org/2000/svg", "line");
    beam.setAttribute("x1", x1);
    beam.setAttribute("y1", y1);
    beam.setAttribute("x2", x2);
    beam.setAttribute("y2", y2);
    beam.setAttribute("stroke", "#f59e0b");
    beam.setAttribute("stroke-width", "2.5");
    beam.setAttribute("stroke-dasharray", "6 4");
    beam.setAttribute("class", "handoff-beam");
    trucksGroup.appendChild(beam);
  }

  // Draw Docking Tether Lines to Destination Center when trucks are safely parked in bays
  if (t1.state === "DELIVERED" && simState.contract.transporterId === "Truck_DL01") {
    const dock1 = document.createElementNS("http://www.w3.org/2000/svg", "line");
    dock1.setAttribute("x1", x1);
    dock1.setAttribute("y1", y1);
    dock1.setAttribute("x2", pDest1.x);
    dock1.setAttribute("y2", pDest1.y);
    dock1.setAttribute("stroke", "rgba(16, 185, 129, 0.45)");
    dock1.setAttribute("stroke-width", "1.5");
    dock1.setAttribute("stroke-dasharray", "3 3");
    trucksGroup.appendChild(dock1);
  }
  if (t2.state === "DELIVERED") {
    const dock2 = document.createElementNS("http://www.w3.org/2000/svg", "line");
    dock2.setAttribute("x1", x2);
    dock2.setAttribute("y1", y2);
    dock2.setAttribute("x2", pDestFinal2.x);
    dock2.setAttribute("y2", pDestFinal2.y);
    dock2.setAttribute("stroke", "rgba(52, 211, 153, 0.45)");
    dock2.setAttribute("stroke-width", "1.5");
    dock2.setAttribute("stroke-dasharray", "3 3");
    trucksGroup.appendChild(dock2);
  }

  // 4. Render Truck DL01
  const g1 = document.createElementNS("http://www.w3.org/2000/svg", "g");
  g1.setAttribute("transform", `translate(${x1}, ${y1})`);

  if (t1.state === "STALLED") {
    // Red pulsing hazard halo
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "18");
    halo.setAttribute("fill", "rgba(239, 68, 68, 0.4)");
    halo.setAttribute("class", "pulse-halo-stall");
    g1.appendChild(halo);

    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "8");
    pin.setAttribute("fill", "#ef4444");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "2");
    g1.appendChild(pin);

    const badge = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    badge.setAttribute("x", "-52");
    badge.setAttribute("y", "-32");
    badge.setAttribute("width", "104");
    badge.setAttribute("height", "18");
    badge.setAttribute("rx", "4");
    badge.setAttribute("fill", "#1e131d");
    badge.setAttribute("stroke", "#ef4444");
    badge.setAttribute("stroke-width", "1.5");
    g1.appendChild(badge);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "-19");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#fca5a5");
    lbl.setAttribute("font-size", "10");
    lbl.setAttribute("font-weight", "bold");
    lbl.textContent = "⚠️ DL01 (STALLED)";
    g1.appendChild(lbl);
  } else if (t1.state === "DELIVERED") {
    // Dedicated Dock Bay 1 Styling (West Bay)
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "13");
    halo.setAttribute("fill", "rgba(16, 185, 129, 0.3)");
    g1.appendChild(halo);

    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "6.5");
    pin.setAttribute("fill", "#10b981");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "1.5");
    g1.appendChild(pin);

    const badge = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    badge.setAttribute("x", "-44");
    badge.setAttribute("y", "-28");
    badge.setAttribute("width", "88");
    badge.setAttribute("height", "18");
    badge.setAttribute("rx", "4");
    badge.setAttribute("fill", "#091428");
    badge.setAttribute("stroke", "#10b981");
    badge.setAttribute("stroke-width", "1.4");
    badge.setAttribute("filter", "drop-shadow(0 2px 5px rgba(0,0,0,0.7))");
    g1.appendChild(badge);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "-15");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#6ee7b7");
    lbl.setAttribute("font-size", "9.5");
    lbl.setAttribute("font-weight", "700");
    lbl.textContent = "✅ DL01 Done";
    g1.appendChild(lbl);
  } else {
    // En Route / Diverted
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "13");
    halo.setAttribute("fill", t1.state === "DIVERTED" ? "rgba(245, 158, 11, 0.3)" : "rgba(16, 185, 129, 0.2)");
    g1.appendChild(halo);

    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "6.5");
    pin.setAttribute("fill", t1.state === "DIVERTED" ? "#f59e0b" : "#10b981");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "1.5");
    g1.appendChild(pin);

    const badge = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    badge.setAttribute("x", "-40");
    badge.setAttribute("y", "-28");
    badge.setAttribute("width", "80");
    badge.setAttribute("height", "16");
    badge.setAttribute("rx", "3");
    badge.setAttribute("fill", "#0f172a");
    badge.setAttribute("stroke", "#334155");
    badge.setAttribute("stroke-width", "1");
    g1.appendChild(badge);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "-16");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#e2e8f0");
    lbl.setAttribute("font-size", "9.5");
    lbl.setAttribute("font-weight", "600");
    lbl.textContent = `🚚 DL01 (${t1.currentProgress.toFixed(0)}%)`;
    g1.appendChild(lbl);
  }
  trucksGroup.appendChild(g1);

  // 5. Render Truck HR02 (Peer Standby / Takeover / Batch 2)
  const g2 = document.createElementNS("http://www.w3.org/2000/svg", "g");
  g2.setAttribute("transform", `translate(${x2}, ${y2})`);

  if (t2.state === "IDLE") {
    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "5.5");
    pin.setAttribute("fill", "#475569");
    pin.setAttribute("stroke", "#94a3b8");
    pin.setAttribute("stroke-width", "1.5");
    g2.appendChild(pin);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "16");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#94a3b8");
    lbl.setAttribute("font-size", "9");
    lbl.textContent = "🚛 HR02 (Standby)";
    g2.appendChild(lbl);
  } else if (t2.state === "EN_ROUTE") {
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "16");
    halo.setAttribute("fill", "rgba(16, 185, 129, 0.35)");
    halo.setAttribute("class", "pulse-halo-peer");
    g2.appendChild(halo);

    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "7.5");
    pin.setAttribute("fill", "#10b981");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "2");
    g2.appendChild(pin);

    const badge = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    badge.setAttribute("x", "-52");
    badge.setAttribute("y", "-32");
    badge.setAttribute("width", "104");
    badge.setAttribute("height", "18");
    badge.setAttribute("rx", "4");
    badge.setAttribute("fill", "#064e3b");
    badge.setAttribute("stroke", "#34d399");
    badge.setAttribute("stroke-width", "1.5");
    g2.appendChild(badge);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "-19");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#a7f3d0");
    lbl.setAttribute("font-size", "10");
    lbl.setAttribute("font-weight", "bold");
    lbl.textContent = isBatch2 ? `🚛 HR02 (BATCH 2: ${t2.currentProgress.toFixed(0)}%)` : `🚛 HR02: TAKEOVER (${t2.currentProgress.toFixed(0)}%)`;
    g2.appendChild(lbl);
  } else if (t2.state === "DIVERTED") {
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "16");
    halo.setAttribute("fill", "rgba(245, 158, 11, 0.35)");
    g2.appendChild(halo);

    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "7.5");
    pin.setAttribute("fill", "#f59e0b");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "2");
    g2.appendChild(pin);

    const badge = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    badge.setAttribute("x", "-52");
    badge.setAttribute("y", "-32");
    badge.setAttribute("width", "104");
    badge.setAttribute("height", "18");
    badge.setAttribute("rx", "4");
    badge.setAttribute("fill", "#2a1a08");
    badge.setAttribute("stroke", "#f59e0b");
    badge.setAttribute("stroke-width", "1.5");
    g2.appendChild(badge);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "-19");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#fcd34d");
    lbl.setAttribute("font-size", "10");
    lbl.setAttribute("font-weight", "bold");
    lbl.textContent = `🚛 HR02: DIVERTED (${t2.currentProgress.toFixed(0)}%)`;
    g2.appendChild(lbl);
  } else if (t2.state === "DELIVERED") {
    // Dedicated Dock Bay 2 Styling (East Bay)
    const halo = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    halo.setAttribute("r", "13");
    halo.setAttribute("fill", "rgba(52, 211, 153, 0.3)");
    g2.appendChild(halo);

    const pin = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    pin.setAttribute("r", "6.5");
    pin.setAttribute("fill", "#059669");
    pin.setAttribute("stroke", "#ffffff");
    pin.setAttribute("stroke-width", "1.5");
    g2.appendChild(pin);

    const badge = document.createElementNS("http://www.w3.org/2000/svg", "rect");
    badge.setAttribute("x", "-46");
    badge.setAttribute("y", "-28");
    badge.setAttribute("width", "92");
    badge.setAttribute("height", "18");
    badge.setAttribute("rx", "4");
    badge.setAttribute("fill", "#091428");
    badge.setAttribute("stroke", "#34d399");
    badge.setAttribute("stroke-width", "1.4");
    badge.setAttribute("filter", "drop-shadow(0 2px 5px rgba(0,0,0,0.7))");
    g2.appendChild(badge);

    const lbl = document.createElementNS("http://www.w3.org/2000/svg", "text");
    lbl.setAttribute("x", "0");
    lbl.setAttribute("y", "-15");
    lbl.setAttribute("text-anchor", "middle");
    lbl.setAttribute("fill", "#34d399");
    lbl.setAttribute("font-size", "9.5");
    lbl.setAttribute("font-weight", "700");
    lbl.textContent = isBatch2 ? "✅ HR02 (B2 Done)" : "✅ HR02 Done";
    g2.appendChild(lbl);
  }
  trucksGroup.appendChild(g2);
}

// ============================================================================
// 7. Interactive Fault Injections & Zero-Leak Recovery
// ============================================================================

function injectMandiGlut() {
  playAlertChime(false);
  simState.nodes["Mandi_Azadpur"].price = 1320;
  simState.nodes["Mandi_Azadpur"].glut = true;
  simState.nodes["Mandi_Azadpur"].occupied = 5; // full capacity

  const oldMandi = simState.contract.mandiId;
  const newMandi = "Mandi_Ghazipur";
  simState.contract.mandiId = newMandi;
  simState.contract.version++;

  const isPeerActive = (simState.contract.transporterId === "Truck_HR02");
  if (isPeerActive) {
    document.getElementById('truck2StandbyText').textContent = "Mandi Glut at Azadpur! Rerouted ➔ Mandi Ghazipur";
  } else {
    document.getElementById('truck1RouteText').textContent = "Farm_Sonipat ➔ Ghazipur";
  }

  // Update Route UI highlights
  updateRouteHighlights();

  // Update Recovery Card
  document.getElementById('recStrategyName').textContent = "DynamicReroutingRecovery (v" + simState.contract.version + ")";
  document.getElementById('recStatusBadge').textContent = "Reroute Succeeded";
  document.getElementById('recStatusBadge').style.background = "rgba(16, 185, 129, 0.25)";
  document.getElementById('recStatusBadge').style.color = "#34d399";
  document.getElementById('recActionSummary').textContent = 
    `Slot cleanly cancelled at Azadpur (Glut: 5/5, Price dropped to ₹1,320). Replacement slot reserved at Ghazipur (Quote: ₹2,100/qtl). Net gain ₹16,500 exceeds hysteresis delta ₹5,000. Old gate slot released; zero leaked resources.`;

  speakHinglishAlert("Mandi update: Azadpur mandi me aamad badhne se bhaav gir gaya hai. Truck Ghazipur mandi reroute kiya ja raha hai.");
  logEvent("RECOVERY", "REROUTE", `DynamicReroutingRecovery: Diverted from ${oldMandi} to ${newMandi}. Slot rebound (contract v${simState.contract.version})`);
  
  updateInvariantMonitor();
  updateTruckPosition();

  if (!simState.running) {
    startSimulation();
  }
}

function injectTruckBreakdown() {
  playAlertChime(false);
  const t1 = simState.trucks[0];
  const t2 = simState.trucks[1];

  // Station DL01 breakdown point at NH-44 Junction rendezvous (50.0%)
  t1.currentProgress = 50.0;
  t1.state = "STALLED";
  document.getElementById('truck1StateBadge').className = "truck-state-badge stalled";
  document.getElementById('truck1StateBadge').textContent = "STALLED (ENGINE FAILURE)";
  document.getElementById('truck1ProgressBar').style.width = `${t1.currentProgress}%`;
  document.getElementById('truck1ProgressBar').style.background = "#ef4444";
  document.getElementById('truck1Eta').textContent = "STALLED";

  speakHinglishAlert("Alert: Truck DL01 engine overheat hone se NH-44 par kharab hua! Multi-Agent Broker peer handoff coordinate kar raha hai...");
  logEvent("FAULT", "ENGINE_OVERHEAT", "Truck_DL01 mechanical breakdown at NH-44 Junction. Telemetry alert dispatched to Broker.");

  updateTruckPosition();

  // Execute PeerHandoffRecovery
  setTimeout(() => {
    t2.state = "EN_ROUTE";
    t2.currentProgress = 50.0; // Starts right at NH-44 Junction rendezvous

    document.getElementById('truck2StateBadge').className = "truck-state-badge enroute";
    document.getElementById('truck2StateBadge').textContent = "ACTIVE (PEER TAKEOVER)";
    document.getElementById('truck2ProgressBar').style.width = "50%";
    document.getElementById('truck2StandbyText').textContent = `Manifest taken over from DL01 ➔ ${simState.contract.mandiId.replace('_', ' ')}`;

    // Atomic 2PC Lock Rebind
    simState.contract.transporterId = t2.id;
    simState.contract.version++;

    document.getElementById('recStrategyName').textContent = "PeerHandoffRecovery (v" + simState.contract.version + ")";
    document.getElementById('recStatusBadge').textContent = "Handoff Complete";
    document.getElementById('recStatusBadge').style.background = "rgba(16, 185, 129, 0.25)";
    document.getElementById('recStatusBadge').style.color = "#34d399";
    document.getElementById('recActionSummary').textContent = 
      `Atomic 2PC handoff: Truck_DL01 lock cleanly released. Cargo manifest & escrow rebound to peer Truck_HR02 at NH44_Junction. Mandi gate slot preserved; zero leaked locks!`;

    speakHinglishAlert("Peer Handoff safal! Truck HR02 ne maal sambhal liya hai aur mandi ke liye ravana ho chuka hai.");
    logEvent("RECOVERY", "PEER_HANDOFF", `PeerHandoffRecovery: Cargo manifest handed off from Truck_DL01 to Truck_HR02 at NH44 (contract v${simState.contract.version})`);
    
    updateInvariantMonitor();
    updateRouteHighlights();
    updateTruckPosition();

    // Auto-resume simulation so peer truck visibly drives towards destination
    if (!simState.running) {
      startSimulation();
    }
  }, 500);
}

function injectHighwayJam() {
  playAlertChime(false);
  const t1 = simState.trucks[0];
  const t2 = simState.trucks[1];

  // Check which transporter currently holds the cargo contract
  const isPeerActive = (simState.contract.transporterId === "Truck_HR02");

  if (isPeerActive) {
    // DL01 is broken down at NH-44. It must stay STALLED!
    // HR02 is carrying the harvest: divert HR02 to Processor_KetchupCo
    if (t2.currentProgress < 50.0) t2.currentProgress = 50.0;
    t2.state = "DIVERTED";
    document.getElementById('truck2StateBadge').className = "truck-state-badge diverted";
    document.getElementById('truck2StateBadge').textContent = "DIVERTED (FACTORY)";
    document.getElementById('truck2StandbyText').textContent = "Highway Jam at NH-44! Diverted to KetchupCo Factory (15km)";
  } else {
    // DL01 is the active transporter
    if (t1.currentProgress < 50.0) t1.currentProgress = 50.0;
    t1.state = "DIVERTED";
    document.getElementById('truck1StateBadge').className = "truck-state-badge diverted";
    document.getElementById('truck1StateBadge').textContent = "DIVERTED (FACTORY)";
    document.getElementById('truck1RouteText').textContent = "NH44_Junction ➔ Processor_KetchupCo";
  }

  // Quality drops to 0.55 breaching 0.60 mandi floor
  simState.batch.currentQuality = 0.55;
  document.getElementById('truck1QualityText').textContent = "0.550 (Distress Grade C)";
  document.getElementById('truck1QualityText').style.color = "#f59e0b";
  updateProduceCard("Tomato", 25.0);

  // Execute DistressAuctionRecovery
  simState.contract.mandiId = "Processor_KetchupCo";
  simState.contract.hasGateSlot = false; // Mandi slot released
  simState.contract.version++;

  // Update Route UI
  updateRouteHighlights();

  document.getElementById('recStrategyName').textContent = "DistressAuctionRecovery (v" + simState.contract.version + ")";
  document.getElementById('recStatusBadge').textContent = "Distress Diversion";
  document.getElementById('recStatusBadge').style.background = "rgba(245, 158, 11, 0.25)";
  document.getElementById('recStatusBadge').style.color = "#fbbf24";
  document.getElementById('recActionSummary').textContent = 
    `Mandi gate slot released immediately. Truck diverted to Processor_KetchupCo (15 km) at distress bid ₹1,850/qtl. 100% produce salvaged from spoilage; zero leaked slot locks.`;

  speakHinglishAlert("Kisan Bhai dhyan dein: Highway jam se bachane ke liye truck tomato ketchup factory divert kiya gaya.");
  logEvent("RECOVERY", "DISTRESS_AUCTION", `DistressAuctionRecovery: Mandi slot freed. Diverted to Processor_KetchupCo at ₹1,850/q (contract v${simState.contract.version})`);
  
  updateInvariantMonitor();
  updateTruckPosition();

  if (!simState.running) {
    startSimulation();
  }
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

function completeContractDelivery(truck) {
  simState.contract.state = "COMPLETED";
  simState.contract.hasTruckLock = false;
  simState.contract.hasGateSlot = false;
  simState.contract.hasEscrow = false;

  if (truck.id === "Truck_DL01") {
    document.getElementById('truck1StateBadge').className = "truck-state-badge completed";
    document.getElementById('truck1StateBadge').textContent = "FULFILLED (DELIVERED)";
    document.getElementById('truck1Eta').textContent = "DELIVERED";
  } else {
    document.getElementById('truck2StateBadge').className = "truck-state-badge completed";
    document.getElementById('truck2StateBadge').textContent = "FULFILLED (PEER DELIVERED)";
    document.getElementById('truck2StandbyText').textContent = "Cargo delivered safely to Mandi. Escrow payment released to Truck_HR02.";
  }

  const isPlant = simState.contract.mandiId === "Processor_KetchupCo";
  const destName = isPlant ? "Mother Dairy Ketchup Plant (Distress Salvage)" : simState.contract.mandiId.replace('_', ' ');

  speakHinglishAlert(`Mubarak ho! Fasal ${destName} safaltapoorvak pahunch gayi hai. Escrow bhuqtaan transporter ko jaari kiya gaya.`);
  logEvent("INFO", "CONTRACT_FULFILLED", `Contract ${simState.contract.id} completed by ${truck.id} at ${destName}. Escrow disbursed, zero resources leaked.`);
  updateInvariantMonitor();
  if (simState.contract2.state !== "COMMITTED") {
    pauseSimulation();
  }
}

function completeBatch2Delivery(truck) {
  simState.contract2.state = "COMPLETED";
  simState.contract2.hasTruckLock = false;
  simState.contract2.hasGateSlot = false;
  simState.contract2.hasEscrow = false;

  document.getElementById('truck2StateBadge').className = "truck-state-badge completed";
  document.getElementById('truck2StateBadge').textContent = "FULFILLED (BATCH 2 DELIVERED)";
  document.getElementById('truck2StandbyText').textContent = "40q Onion delivered safely to Ghazipur Mandi. Transporter escrow disbursed.";

  speakHinglishAlert("Mubarak ho! Doosri fasal (Batch 2: 40q Pyaaz) safaltapoorvak Ghazipur mandi pahunch gayi hai. Escrow bhuqtaan jaari kiya gaya.");
  logEvent("INFO", "CONTRACT_FULFILLED", `Concurrent Batch 2 (CTR-102) completed by ${truck.id} at Mandi_Ghazipur. Escrow disbursed, zero resources leaked.`);
  updateInvariantMonitor();
  if (simState.contract.state === "COMPLETED" || simState.contract.state === "ABORTED") {
    pauseSimulation();
  }
}

function tickSimulation(stepMinutes = 15) {
  simState.timeMinutes += stepMinutes;

  const hrs = String(Math.floor(simState.timeMinutes / 60)).padStart(2, '0');
  const mins = String(simState.timeMinutes % 60).padStart(2, '0');
  document.getElementById('simTimeDisplay').textContent = `${hrs}h : ${mins}m`;
  document.getElementById('simTickDisplay').textContent = `T+${simState.timeMinutes}m (Seed ${simState.seed})`;

  // Freshness decay calculation
  const hoursElapsed = (simState.timeMinutes - simState.batch.harvestTime) / 60.0;
  const decay = simState.batch.decayLambda * simState.batch.tempMultiplier;
  simState.batch.currentQuality = Math.max(0.40, simState.batch.initialQuality * Math.exp(-decay * hoursElapsed));
  document.getElementById('truck1QualityText').textContent = 
    `${simState.batch.currentQuality.toFixed(3)} (${simState.batch.currentQuality >= 0.8 ? 'Grade A' : 'Grade B'})`;
  updateProduceCard(simState.batch.commodity, simState.batch.volume);

  const t1 = simState.trucks[0];
  const t2 = simState.trucks[1];

  // 1. Progress Truck DL01 if active and holding the contract
  if ((t1.state === "EN_ROUTE" || t1.state === "DIVERTED") && simState.contract.transporterId === "Truck_DL01") {
    t1.currentProgress = Math.min(100.0, t1.currentProgress + (stepMinutes * 0.45));
    document.getElementById('truck1ProgressBar').style.width = `${t1.currentProgress}%`;
    const remainingPct = 100 - t1.currentProgress;
    document.getElementById('truck1Eta').textContent = `${Math.max(0, Math.round(remainingPct * 3.6))}m`;

    if (t1.currentProgress >= 100.0 && simState.contract.state !== "COMPLETED") {
      t1.state = "DELIVERED";
      completeContractDelivery(t1);
    }
  }

  // 2. Progress Truck HR02 if active (en route or diverted)
  if (t2.state === "EN_ROUTE" || t2.state === "DIVERTED") {
    t2.currentProgress = Math.min(100.0, t2.currentProgress + (stepMinutes * 0.45));
    document.getElementById('truck2ProgressBar').style.width = `${t2.currentProgress}%`;
    const remainingPct = 100 - t2.currentProgress;
    const etaMins = Math.max(0, Math.round(remainingPct * 3.6));

    if (simState.contract2.state === "COMMITTED") {
      document.getElementById('truck2StandbyText').textContent = `Batch 2 (40q Onion): ${t2.currentProgress.toFixed(0)}% en route ➔ Ghazipur (ETA: ${etaMins}m)`;
      if (t2.currentProgress >= 100.0 && simState.contract2.state !== "COMPLETED") {
        t2.state = "DELIVERED";
        completeBatch2Delivery(t2);
      }
    } else {
      const destLabel = simState.contract.mandiId === "Processor_KetchupCo" ? "Mother Dairy Plant" : simState.contract.mandiId.replace('_', ' ');
      const actionLabel = t2.state === "DIVERTED" ? "Diverting to Factory" : "Taking over";
      document.getElementById('truck2StandbyText').textContent = `${actionLabel}: ${t2.currentProgress.toFixed(0)}% en route ➔ ${destLabel} (ETA: ${etaMins}m)`;
      if (t2.currentProgress >= 100.0 && simState.contract.state !== "COMPLETED") {
        t2.state = "DELIVERED";
        completeContractDelivery(t2);
      }
    }
  }

  updateTruckPosition();
}

function startSimulation() {
  if (simState.running) return;
  simState.running = true;
  document.getElementById('playPauseText').textContent = "Pause";
  document.getElementById('playIcon').innerHTML = '<rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/>';

  const intervalMs = Math.max(150, 800 / simState.speed);
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
    simState.contract.version = 1;
    simState.contract.transporterId = "Truck_DL01";
    simState.contract.hasTruckLock = true;
    simState.contract.hasGateSlot = true;
    simState.contract.hasEscrow = true;
    simState.contract.mandiId = "Mandi_Azadpur";

    simState.nodes["Mandi_Azadpur"].price = 2200;
    simState.nodes["Mandi_Azadpur"].glut = false;
    simState.nodes["Mandi_Azadpur"].occupied = 2;

    simState.trucks[0].state = "EN_ROUTE";
    simState.trucks[0].currentProgress = 45.0;
    simState.trucks[1].state = "IDLE";
    simState.trucks[1].currentProgress = 0.0;

    document.getElementById('truck1ProgressBar').style.width = '45%';
    document.getElementById('truck1ProgressBar').style.background = '';
    document.getElementById('truck1StateBadge').className = 'truck-state-badge enroute';
    document.getElementById('truck1StateBadge').textContent = 'EN_ROUTE';
    document.getElementById('truck1RouteText').textContent = 'Farm_Sonipat ➔ Azadpur';
    document.getElementById('truck1Eta').textContent = '180m';

    document.getElementById('truck2ProgressBar').style.width = '0%';
    document.getElementById('truck2StateBadge').className = 'truck-state-badge idle';
    document.getElementById('truck2StateBadge').textContent = 'IDLE (STANDBY)';
    document.getElementById('truck2StandbyText').textContent = 'Ready for Peer Handoff';

    simState.contract2.state = "IDLE";
    simState.contract2.version = 1;
    simState.contract2.hasTruckLock = false;
    simState.contract2.hasGateSlot = false;
    simState.contract2.hasEscrow = false;

    const inspector = document.getElementById('nodeInspectorCard');
    if (inspector) inspector.classList.add('hidden');

    document.getElementById('recStrategyName').textContent = 'Ready (No Active Disruptions)';
    document.getElementById('recStatusBadge').textContent = 'Idle';
    document.getElementById('recStatusBadge').style.background = '';
    document.getElementById('recStatusBadge').style.color = '';
    document.getElementById('recActionSummary').textContent = 'Trigger a fault injection above to observe multi-agent compensation and zero-leak recovery protocols.';

    initRoadMap();
    updateInvariantMonitor();
    updateTruckPosition();
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

  // DTMF Numpad Buttons (Interactive Telephone IVR)
  document.querySelectorAll('.numpad-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const key = btn.dataset.key;
      playDtmfTone(key);
      if (key === '1') {
        simState.batch.commodity = "Tomato";
        document.getElementById('parsedCommodity').textContent = "Tamatar (Tomato)";
        updateProduceCard("Tomato", simState.batch.volume);
      } else if (key === '2') {
        simState.batch.commodity = "Onion";
        document.getElementById('parsedCommodity').textContent = "Pyaaz (Onion)";
        updateProduceCard("Onion", simState.batch.volume);
      } else if (key === '3') {
        simState.batch.commodity = "Potato";
        document.getElementById('parsedCommodity').textContent = "Aalu (Potato)";
        updateProduceCard("Potato", simState.batch.volume);
      } else if (key === '4') {
        document.getElementById('parsedDestination').textContent = "Azadpur Mandi";
      } else if (key === '5') {
        document.getElementById('parsedDestination').textContent = "Ghazipur Mandi";
      } else if (key === '6') {
        document.getElementById('parsedDestination').textContent = "Sonipat Processing Plant";
      } else if (key === '7') {
        simState.batch.volume = 25.0;
        document.getElementById('parsedQuantity').textContent = "25.0 Quintals";
        updateProduceCard(simState.batch.commodity, 25.0);
      } else if (key === '8') {
        simState.batch.volume = 50.0;
        document.getElementById('parsedQuantity').textContent = "50.0 Quintals";
        updateProduceCard(simState.batch.commodity, 50.0);
      } else if (key === '9' || key === '#') {
        document.getElementById('btnConfirm2PC').click();
        return;
      } else if (key === '*') {
        document.getElementById('btnCancel2PC').click();
        return;
      }
      logEvent("INFO", "DTMF_IVR", `DTMF Key [${key}] selected: ${simState.batch.commodity} (${simState.batch.volume}q) ➔ ${document.getElementById('parsedDestination').textContent}`);
    });
  });

  // 2PC Buttons
  document.getElementById('btnConfirm2PC').addEventListener('click', () => {
    playAlertChime(true);
    simState.contract.state = "COMMITTED";
    simState.contract.hasTruckLock = true;
    simState.contract.hasGateSlot = true;
    simState.contract.hasEscrow = true;
    simState.contract.version++;

    // Map parsed destination to node ID
    const parsedDestText = document.getElementById('parsedDestination').textContent;
    if (parsedDestText.includes("Ghazipur")) {
      simState.contract.mandiId = "Mandi_Ghazipur";
    } else if (parsedDestText.includes("Sonipat") || parsedDestText.includes("Plant")) {
      simState.contract.mandiId = "Processor_KetchupCo";
    } else {
      simState.contract.mandiId = "Mandi_Azadpur";
    }

    const t1 = simState.trucks[0];
    t1.state = "EN_ROUTE";
    t1.currentProgress = 0.0;
    document.getElementById('truck1ProgressBar').style.width = '0%';
    document.getElementById('truck1ProgressBar').style.background = '';
    document.getElementById('truck1StateBadge').className = 'truck-state-badge enroute';
    document.getElementById('truck1StateBadge').textContent = 'EN_ROUTE';
    document.getElementById('truck1RouteText').textContent = `Farm_Sonipat ➔ ${simState.contract.mandiId.replace('_', ' ')}`;
    document.getElementById('truck1Eta').textContent = '220m';

    // Update active route line
    const edgeAzad = document.getElementById('edge_NH44_Junction_Mandi_Azadpur');
    const edgeGhaz = document.getElementById('edge_NH44_Junction_Mandi_Ghazipur');
    const edgePlant = document.getElementById('edge_NH44_Junction_Processor_KetchupCo');
    if (edgeAzad) edgeAzad.className.baseVal = simState.contract.mandiId === "Mandi_Azadpur" ? "map-edge active-route" : "map-edge";
    if (edgeGhaz) edgeGhaz.className.baseVal = simState.contract.mandiId === "Mandi_Ghazipur" ? "map-edge active-route reroute" : "map-edge";
    if (edgePlant) edgePlant.className.baseVal = simState.contract.mandiId === "Processor_KetchupCo" ? "map-edge active-route reroute" : "map-edge";

    updateInvariantMonitor();
    updateTruckPosition();

    speakHinglishAlert(`Dhanyavaad! 2-Phase Commit safal raha. ${simState.batch.volume}q ${simState.batch.commodity} ke liye truck aur ${simState.contract.mandiId.replace('_', ' ')} slot reserve ho gaye hain.`);
    logEvent("INFO", "2PC_COMMIT", `2PC Commit Succeeded: ${simState.batch.volume}q ${simState.batch.commodity} locked for ${simState.contract.mandiId} (Contract v${simState.contract.version}).`);
    
    // Auto-start simulation so the journey starts
    startSimulation();
  });

  document.getElementById('btnCancel2PC').addEventListener('click', () => {
    playAlertChime(false);
    pauseSimulation();
    simState.contract.state = "ABORTED";
    simState.contract.hasTruckLock = false;
    simState.contract.hasGateSlot = false;
    simState.contract.hasEscrow = false;
    simState.trucks[0].state = "IDLE";
    document.getElementById('truck1StateBadge').className = "truck-state-badge idle";
    document.getElementById('truck1StateBadge').textContent = "CANCELLED (ABORTED)";
    updateInvariantMonitor();
    updateTruckPosition();
    speakHinglishAlert("Aadesh radd kar diya gaya hai. Sabhi resource locks release ho chuke hain.");
    logEvent("ROLLBACK", "2PC_ABORT", "Farmer cancelled request. Reverse compensation executed with zero leaks.");
  });

  // Option 1: Concurrent Multi-Harvest Batch 2 Dispatch
  const btnBatch2 = document.getElementById('btnDispatchBatch2');
  if (btnBatch2) {
    btnBatch2.addEventListener('click', () => {
      simState.contract2.state = "COMMITTED";
      simState.contract2.hasTruckLock = true;
      simState.contract2.hasGateSlot = true;
      simState.contract2.hasEscrow = true;
      simState.contract2.version = 1;
      simState.contract2.mandiId = "Mandi_Ghazipur";

      const t2 = simState.trucks[1];
      t2.state = "EN_ROUTE";
      t2.currentProgress = 0.0;
      t2.routeText = "Farm_Sonipat ➔ Ghazipur";

      document.getElementById('truck2StateBadge').className = "truck-state-badge enroute";
      document.getElementById('truck2StateBadge').textContent = "EN_ROUTE (BATCH 2: 40q ONION)";
      document.getElementById('truck2ProgressBar').style.width = "0%";
      document.getElementById('truck2StandbyText').textContent = "Batch 2: 40.0q Onion en route to Ghazipur Mandi";

      // Highlight routes on map
      const edgeAzad = document.getElementById('edge_NH44_Junction_Mandi_Azadpur');
      const edgeGhaz = document.getElementById('edge_NH44_Junction_Mandi_Ghazipur');
      if (edgeAzad && simState.contract.mandiId === "Mandi_Azadpur") edgeAzad.className.baseVal = "map-edge active-route";
      if (edgeGhaz) edgeGhaz.className.baseVal = "map-edge active-route";

      updateInvariantMonitor();
      updateTruckPosition();

      speakHinglishAlert("Doosri fasal (Batch 2: 40q Pyaaz) Ghazipur Mandi ke liye Truck HR02 ke saath dispatch ho chuki hai.");
      logEvent("INFO", "CNP_2PC", "Multi-Harvest Batch 2 (CTR-102: 40q Onion) committed atomically to Truck_HR02 for Mandi_Ghazipur. 2 concurrent shipments active!");

      startSimulation();
    });
  }

  // Option 3: Node Inspector Close Button
  const closeInspBtn = document.getElementById('btnCloseInspector');
  if (closeInspBtn) {
    closeInspBtn.addEventListener('click', () => {
      const inspector = document.getElementById('nodeInspectorCard');
      if (inspector) inspector.classList.add('hidden');
    });
  }

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
