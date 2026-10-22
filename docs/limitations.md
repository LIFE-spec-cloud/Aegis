# KisanSetu (MandiFlow) - System Limitations & Assumptions

This document lists the technical assumptions, environmental simplifications, and known constraints of the KisanSetu simulation engine.

---

## 1. Network & Topology Assumptions

1. **Discrete Graph Representation**:
   - Road routes are modeled as edges connecting discrete node hubs (farms, junctions, APMC mandis, processing plants).
   - In-transit position interpolates along current edge transit time, rather than fine-grained GPS road curvature.

2. **Edge Congestion & Toll Granularity**:
   - Congestion multipliers update deterministically per edge. Dynamic weather micro-climates along a single highway segment are treated as uniform across the edge.

---

## 2. Produce Decay & Thermal Physics

1. **Piecewise Uniform Ambient Temperature**:
   - The thermal multiplier $\mu_{\text{temp}}$ follows a linear profile between $20^\circ\text{C}$ and $40^\circ\text{C}$.
   - Diurnal thermal cycles are modeled at hourly simulation steps. Sudden microclimate shock (e.g. flash hail or abrupt localized heat waves) is not modeled dynamically.

2. **Homogeneous Batch Freshness**:
   - All produce in a given `CropBatch` begins at initial quality $Q_0 = 1.0$ and decays uniformly. Varietal differences within the same commodity type (e.g., hybrid vs desi tomato) are parameterized by a single calibrated decay parameter $\lambda$.

---

## 3. Market & Economic Mechanics

1. **Surge Price Slope Linearity**:
   - The mandi surge pricing model clamps at a floor of $40\%$ base price, falling with a constant slope factor $k = 0.50$ when inbound volume exceeds hourly absorption quota. Real-world mandi auctions may experience non-linear step-function crashes or sudden cartel collusion.

2. **Escrow & Financial Clearance**:
   - All monetary calculations are performed in integer paise (strong type `Money`) without rounding or float drift.
   - Escrow holds are assumed to clear instantly without inter-bank settlement delays or credit defaults.

---

## 4. Messaging & Concurrency

1. **Lossless Default with Controlled Jitter**:
   - By default, discrete-event delivery is lossless and in-order to ensure baseline mathematical reproducibility.
   - Configurable Bernoulli packet loss and simulated latency are available for fault injection tests.

2. **Serialized Broker Commits**:
   - In parallel mode (`--parallel`), while agents process mailboxes concurrently, the `LogisticsBroker` serializes contract commits. This design intentionally prevents double-booking of physical gate slots and truck holds.

3. **Simulated Vernacular Voice Interface**:
   - Spoken Hindi/Hinglish commands are parsed via deterministic token matching against a vernacular lexicon rather than acoustic speech-to-text neural models. Low-confidence inputs generate confirmation prompts.
