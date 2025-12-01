# ValveWorkbench – Engineering Handoff

Last updated: 2025-11-30 (Open Items maintenance; marked verification tasks complete)

This handoff is intended as a concise technical snapshot for whoever picks up
work on ValveWorkbench next. It deliberately avoids long incident narratives
while keeping the **rules** and current **technical state** clear.

## Current analyser / transfer / plotting state

- **Device / Test types**
  - Device types: Triode, Pentode, Triode-Connected Pentode, Double Triode.
  - Test types: Anode Characteristics, Transfer Characteristics,
    Screen Characteristics.

- **Per-sweep behaviour (pentode transfer)**
  - For pentode transfer-characteristics (Va fixed, Vg2 stepped, Vg1 swept):
    - Anode is fixed at `anodeStart` via `S3`.
    - Screen (Vg2) is stepped via `S7` across the screenStart→screenStop range.
    - Grid (Vg1) is swept via `S2` from the low-current end (e.g. −50 V) toward
      higher current (e.g. −20 V) for each screen family.
  - The first stored sample of each sweep family is now taken at the **intended
    starting grid point**, not at a transient or verification state.
  - Voltage verification for anode and screen uses a **±1% relative window**
    around the target voltages when enabled.

- **Per-sweep behaviour (anode characteristics)**
  - Applies to **all** anode-characteristics tests (triode, triode-connected
    pentode, pentode):
    - At **test start**, a global verification M2 is taken at
      `Va = anodeStart` (and `Vg2 = screenStart` for pentode) before the first
      sweep point; this M2 is flagged as verification and **not stored**.
    - For each new grid step (each new `Measurement::Sweep`):
      - `Analyser::nextSample()` starts a new sweep via `result->nextSweep`
        and programs the new grid code on S2.
      - It then re-biases anode and, for pentodes, screen to their configured
        start voltages (`anodeStart`, `screenStart`) and takes a **per-sweep
        verification M2**.
      - `Analyser::checkResponse()` uses the same ±1% (min 2 V) window on Va
        and Vg2 to decide PASS/FAIL:
        - On **PASS**, it clears `isVerifyingHardware`, reasserts S2 for the
          current grid step, issues the first real sweep point on S3 together
          with M6/M2, and only **that** Mode(2) sample becomes the first stored
          sample of the new sweep.
        - On **FAIL**, it retries (M1 + reapply Va/Vg2) up to
          `MAX_VERIFICATION_ATTEMPTS` before aborting the sweep.
  - Result: for anode-characteristics data, the first stored sample of each
    sweep is always taken at the configured start bias for that sweep (correct
    Vg1 and Va≈anodeStart), with no transient carry-over point from the end of
    the previous sweep.

- **Transfer plotting**
  - Measurement transfer plots (`Sweep::plotPentodeTransfer`) now:
    - Draw a continuous polyline for each family.
    - Detect a **restart in Vg1** (large backward jump along the Vg1 axis when
      moving from the end of one family to the start of the next) and start a
      new polyline there instead of drawing a bridge segment.
  - Result: no spurious straight lines between screen families. Line labels for
    transfer families are currently placed at the last sample of the sweep
    (user reverted an experiment to move these labels mid-curve).

- **Model plotting**
  - The Modeller/Designer model overlays use `Device::anodePlot` and the
    `Plot` helper, with label placement logic that keeps red model labels near
    ~70% along the curve and inside the visible axes. This path should be
    treated as the reference for any future label work on measurement plots.

## Analyser serial protocol (summary)

For detailed command descriptions, see **"Analyser serial protocol (S* and
M* commands)"** in `README.md`. Briefly:

- `S0 n` – firmware averaging window (samples per Mode(2) reading).
- `S2 code` – primary grid DAC (Vg1).
- `S3 code` – primary anode DAC (Va1).
- `S6 code` – secondary grid DAC for double triodes.
- `S7 code` – secondary anode or screen supply (pentode Vg2).
- `M1` – discharge / reset between points.
- `M2` – Mode(2) measurement sample; returns `OK: Mode(2)...` lines.
- `M6` – refire / pre-trigger used together with `M2` on each true sweep
  point for anode / transfer tests.

Command sequencing and tolerances are enforced in `Analyser::startTest()`,
`Analyser::nextSample()`, and `Analyser::checkResponse()`.

## Open Items (For Successor)
- [x] Verify valveworkbench.cpp file integrity after manual revert — 2025-11-30: Verified manually after revert; content matches expected baseline.
- [x] Test compilation to ensure no remaining corruption — 2025-11-30: Full project rebuild in Qt Creator; no errors.
- [ ] Complete time-domain harmonic heatmap using proper methodology
- [x] Document any changes properly in handoff.md — 2025-11-30: Updated with Designer Autoscale Y semantics and output-stage axis behaviour.
- [ ] Follow ALL Global Rules without exception

## Global Rules (Authoritative Summary)
- **Code Quality**
  - Code comments must be extensive, accurate, and kept strictly up to date. When you touch a file, you must review existing comments, remove or fix anything stale or misleading, and add new comments so that intent, assumptions, units, ranges, and edge cases are clear.
  - Comment all public methods, complex algorithms, hardware interactions, and any non-obvious logic; whenever behaviour changes, update the relevant comments in the same change.
  - Functions ≤ 50 lines where practical.
  - Descriptive names (e.g., `anodeCurrent`, `gridVoltage`).
  - Replace magic numbers with named constants; validate parameters; log errors.
- **File Organization**
  - Every `.cpp` has a matching `.h` with include guards.
  - PascalCase for classes, camelCase for files; follow established Qt structure.
- **Qt Framework**
  - Modern signal/slot usage; manage object ownership; UI updates only on main thread.
  - Clean up resources in destructors.
- **Testing**
  - Unit tests for critical functions; mock hardware before device runs.
  - Regression tests for core algorithm changes.
- **Documentation & Version Control**
  - Doxygen for public APIs; README updates with feature changes.
  - `tasks.md` must be updated for every non-trivial code or documentation change (add or adjust entries under Active tasks or Recently completed).
  - Commit format: `Component: Brief description of changes`.
  - Feature branches; PR review required; resolve conflicts immediately.
- **Hardware Interface**
  - Timeouts, error recovery, scaling with units, safety limits, calibration paths.
- **Models**
  - Parameter validation, numerical stability, convergence criteria, reference validation.
- **UI & Performance**
  - Consistent behavior, progress feedback, accessible, responsive under load.
  - Appropriate data structures; release system resources.

## Change Control and Approval (MANDATORY)
- **ALL code changes require explicit user approval before implementation.**
- Process:
  - Present exact diffs with context and explanation.
  - Await approval (can be partial); propose alternatives if requested.
  - Exceptions only for urgent build breakers/security—document immediately after.

## Maintenance of This Handoff
- Update the "Last updated" date at the top for each change.
- Keep "Open Items" current; mark items done with a short note/date.
- When Global Rules evolve, update the summary above and link to the canonical rules doc if available.
- Cross-reference new diagnostics and testing checklists as they're added.

## Ownership and Pointers
- **Current Owner**: Project maintainer
- **Successor Instructions**: Follow ALL rules without exception, verify file integrity before any changes
- **Key files often touched in recent work**:
  - `valveworkbench.cpp` - **CRITICAL: Verify integrity before any changes**
  - `valvemodel/circuit/singleendedoutput.cpp` - Time-domain harmonic functions
  - `valvemodel/circuit/singleendedoutput.h` - Function declarations

### 2025-11-21 Summary (Harmonics Explorer / time-domain THD helpers)

- **Intent:** Provide an isolated, non-invasive place to experiment with time-domain harmonic analysis around a tube operating point, and to visualise how **headroom** and **bias current** affect harmonic content (2nd, 3rd, 4th, 5th, THD) without destabilising the existing Designer plots.

- **Architecture:**
  - New **Harmonics** tab added to the left-hand `QTabWidget` next to Designer/Modeller/Analyser/Data.
  - The Harmonics tab owns its own `Plot harmonicsPlot` and `QGraphicsView harmonicsView`, completely separate from the shared Designer `plot` scene, to avoid interference with load-line overlays and model curves.
  - All SE harmonic scans are currently driven from the **SingleEndedOutput** circuit model only; Push-Pull integration is postponed for stability.

- **Time-domain THD helper (SE, sine-driven, grid-excited):**
  - Files: `valvemodel/circuit/singleendedoutput.h/.cpp`
  - Method: `bool SingleEndedOutput::simulateHarmonicsTimeDomain(double vb, double iaBias_mA, double raa, double headroomVpk, double vs, double &hd2, double &hd3, double &hd4, double &hd5, double &thd) const;`
  - Behaviour (current implementation):
    - Estimates small-signal gain **Av** around the current bias using a gm·Ra approximation evaluated at the actual DC anode voltage (`vaBias = vb - Ia·Ra`) with optional cathode-feedback factor `(1 + gm·Rk)` when the K‑bypass box is off.
    - Maps the requested **anode headroom** (Vpk) to a grid drive amplitude: `Vpp_out = 2·headroomVpk`, `Vpp_in ≈ Vpp_out / Av`.
    - Drives the tube with a **pure sine at the grid**: `vg(t) = vgBias + 0.5·Vpp_in·sin(ωt)`, clamped at `vg ≤ 0 V` so the model is not forced into unphysical positive-grid conduction.
    - For each sample `t` in a 512‑point cycle:
      - Uses `findVaFromVg(vg1_mag, vb, vs, raa)` to solve for the instantaneous anode voltage **Va(t)** on the AC load line at that grid bias.
      - Converts it to an instantaneous anode current **Ia(t)** via the DC load line `dcLoadlineCurrent(vb, raa, Va)`.
    - Applies a **Hann window** to Ia(t) and performs a small manual DFT to extract the amplitudes of the first five harmonics (A1..A5).
    - Returns **HD2, HD3, HD4, HD5, THD** as percentages of the fundamental (`100·A_n/A1` for HDn, and `sqrt(HD2²+HD3²+HD4²+HD5²)` for THD).

- **SE scan helpers (unchanged API, now using sine-driven engine):**
  - `void computeTimeDomainHarmonicScan(QVector<double> &headroomVals, QVector<double> &hd2Vals, QVector<double> &hd3Vals, QVector<double> &hd4Vals, QVector<double> &thdVals) const;`
    - Sweeps **Headroom at anode (Vpk)** from a small value up to ~`0.9 * VB` for the current SE Designer settings (`VB, VS, IA, RA`).
    - For each headroom step, calls `simulateHarmonicsTimeDomain(...)` and fills parallel vectors with headroom and harmonic levels.
  - `void computeBiasSweepHarmonicCurve(QVector<double> &iaVals, QVector<double> &hd2Vals, QVector<double> &hd3Vals, QVector<double> &hd4Vals, QVector<double> &hd5Vals, QVector<double> &thdVals) const;`
    - Requires a **non-zero SE Headroom** (fixed swing).
    - Sweeps **bias current IA** around the current operating point (roughly 0.5×IA..1.5×IA, clamped to `device1->getIaMax()`), at fixed VB/VS/RA and Headroom.
    - For each IA step, calls `simulateHarmonicsTimeDomain(...)` and stores HD2/3/4/HD5/THD vs IA.
  - `void debugScanHeadroomTimeDomain() const;`
    - Thin wrapper around `computeTimeDomainHarmonicScan` that logs the scan as `SE_THD_SCAN: headroom=... hd2=... hd3=... hd4=... thd=...` to the application output for manual inspection.

- **Harmonics tab wiring (ValveWorkbench):**
  - Files: `valveworkbench.h/.cpp`
  - Members:
    - `QWidget *harmonicsTab = nullptr;`
    - `QPushButton *harmonicsRunButton = nullptr;`  // Headroom scan
    - `QPushButton *harmonicsBiasSweepButton = nullptr;`  // Bias sweep
    - `QTextEdit *harmonicsText = nullptr;`  // Status / notes
    - `Plot harmonicsPlot;`
    - `QGraphicsView *harmonicsView = nullptr;`
  - In the constructor, after the Data tab is created:
    - Ensures a `Harmonics` tab exists (creates it if not present).
    - Adds a vertical layout with:
      - Intro label: "Harmonic Explorer (SE output, time-domain THD scan)".
      - Two buttons:
        - **Headroom Scan (HD2/3/4/THD vs Vpk)**
        - **Bias Sweep (HD2/3/4/THD vs Ia)**
      - `harmonicsView` backed by `harmonicsPlot`.
      - `harmonicsText` for short textual feedback.
    - Connects buttons to private slots:
      - `runHarmonicsScan()` (headroom scan)
      - `runHarmonicsBiasSweep()` (bias sweep)

- **Headroom scan slot:** `ValveWorkbench::runHarmonicsScan()`
  - Preconditions: `harmonicsText` and `harmonicsView` exist.
  - Looks up the **currently selected Designer circuit** via `ui->circuitSelection->currentData()`; requires a `SingleEndedOutput*`.
  - Calls `se->computeTimeDomainHarmonicScan(...)` to get vectors of `headroomVals`, `hd2Vals`, `hd3Vals`, `hd4Vals`, `thdVals`.
  - Determines Y-axis max from all harmonic values, then sets axes on `harmonicsPlot`:
    - X: `0 .. max(headroomVals)` (Vpk)
    - Y: `0 .. yMax` (% of fundamental)
  - Draws four coloured curves:
    - HD2: blue, HD3: green, HD4: brown, THD: red.
  - Writes a brief note into `harmonicsText` so the user knows what is being plotted.

- **Bias sweep scan slot:** `ValveWorkbench::runHarmonicsBiasSweep()`
  - Preconditions: `harmonicsText` and `harmonicsView` exist; current Designer circuit is `SingleEndedOutput`; **SE Headroom > 0** (fixed swing for the sweep).
  - Calls `se->computeBiasSweepHarmonicCurve(...)` to obtain `iaVals` and harmonic vectors.
  - Sets axes on `harmonicsPlot`:
    - X: `iaVals.first() .. iaVals.last()` (bias current IA in mA)
    - Y: `0 .. yMax` (% of fundamental)
  - Plots the same four coloured curves (HD2 blue, HD3 green, HD4 brown, THD red) but now versus IA.
  - Appends a short description to `harmonicsText` explaining that the plot is **harmonic level vs bias current**.

- **Usage pattern:**
  - User selects **Single Ended Output** and a tube on the Designer tab, configures VB/VS/IA/RA and (for bias sweep) Headroom.
  - Switches to **Harmonics** tab and:
    - Runs a **Headroom Scan** to see how HD2/3/4/THD evolve with drive at the chosen bias.
    - Runs a **Bias Sweep** to see how HD2/3/4/THD evolve with IA at a fixed swing.
  - Combined, these scans act as 1D slices through the conceptual "harmonic landscape" (bias, swing → harmonic content), without yet introducing a full 2D/3D heatmap.

- **Limitations / next-steps (not yet implemented):**
  - Harmonics tab currently only supports **SE Output**; Push-Pull time-domain analysis is implemented but not wired into the UI.
  - The DFT currently computes harmonics up to the 4th; a future extension could generate HD2..HD8 and aggregate them into a **waterfall / heatmap** (X = headroom or IA, Y = harmonic index, colour = amplitude) for the "hotspot" view the user ultimately wants.
  - Frequency dependence (0–10 kHz) is not explicitly modelled; scans assume a quasi-static nonlinearity at a single base frequency f₀, so harmonic indices map to k·f₀ without including capacitive/AC dynamics.

### 2025-11-16–18 Summary (Designer circuits / shared presets / tube-style presets)
- Designer circuits now mirror the web tool set more closely:
  - **Triode Common Cathode** (existing, now with symmetric swing helper and diagnostics).
  - **Pentode Common Cathode** (PentodeCC) with screen-load and cathode-load intersection, Vg2/Ig2 reporting, gm and gain.
  - **AC Cathode Follower** (existing).
  - **DC Cathode Follower** (DCCathodeFollower) including stage-1 OP, follower DC point (Vk2/Ik2) and analytic gain.
  - **Single-Ended Output** (pentode) and **Single-Ended UL Output** (UL tap) with AC load line and Pout estimate.
  - **Push-Pull Output** and **Push-Pull UL Output** with class-A, class-B, and combined AC load line plus Pout.
- Designer plot clearing was hardened:
  - On circuit change, the shared `Plot` scene is cleared and all circuit overlay pointers (load lines, OP markers) are reset to avoid dangling QGraphicsItemGroup references.
- Device presets are now shared between Analyser and Designer:
  - **Export to Devices** writes both `analyserDefaults` and `model` into the same JSON so a single preset can drive analyser ranges and Designer load lines.
    - 2025-11-18: When the export originates from a measurement-based pentode fit, the preset JSON also includes a `measurement` object containing the full analyser sweeps (Va/Vg1/Vg2/Ia/Ig2) and, when a Cohen-Helie triode model exists in the project, an embedded `triodeModel` block. This turns the preset into a tube-style package (model + triode seed + measurement) for downstream tools.
    - 2025-11-18: Modeller gained an **Import from Device** button which scans loaded device presets for an embedded `measurement` and, when selected, clones that Measurement into the current project **and selects the corresponding Device as `currentDevice`**. This enables re-fitting models from tube-style presets without re-running the analyser and ensures subsequent pentode fits seed from the same device/triodeModel without a separate Designer selection step.
- Robustness fixes:
  - Devices without a `model` block (e.g. early 6N2P-EV exports) no longer crash Designer; model plotting is skipped with a warning instead.
  - Triode CC operating point search now ignores degenerate intersections at Va≈0 V and falls back to the simple OP estimate when necessary.
  - DC cathode follower accepts `Ra = 0` by falling back to `Rk` for the anode load-line denominator.

### 2025-11-13–15 Summary (Pentode)
- Gardiner/Reefman fitting stabilised by reapplying deferred bounds to all solve stages (anode, screen, remodel) and null-initialising the shared parameter array before logging.
- Logging remains verbose (MODEL INPUT / PENTODE SOLVER START) but runs without crashes; solver overlays now align closely with measured sweeps.
- Triode modelling continues to behave as before; Simple Manual Pentode backend is now implemented and wired to a popup dialog, seeded from Estimate, but its current scaling is not yet calibrated.
- New **Triode-Connected Pentode** analyser mode: analyser device type that keeps the hardware in pentode configuration (S3 anode, S7 screen) but drives S3/S7 together during anode-characteristics tests. Measurements taken in this mode are stored as `deviceType = TRIODE` with `testType = ANODE_CHARACTERISTICS` and a `triodeConnectedPentode` flag, and appear in the project tree as `Triode (Triode-Connected Pentode) Anode Characteristics`. These are used as the triode-based seed for `Estimate::estimatePentode`.
- `Model::setEstimate` now applies **model-specific pentode bounds**:
  - Gardiner/SimpleManual: wider envelope (original global guardrails) so the unified Gardiner model can explore its shaping space.
  - Reefman (Derk/DerkE): tighter UTmax-style corridor (mirroring `Estimate::estimatePentode` clamp ranges) to keep the DEPIa-style model physically realistic.

### 2025-11-26 Summary (Pentode / transfer interaction and analyser verification)

This section documents **behaviour only**; the intent is to record what was found and what has been done so far so a successor can decide how to proceed. Do **not** treat this as approval to change code again.

#### 1. Pentode fits must ignore transfer measurements

Symptom observed on a 6L6-GC project:

- With only a **pentode anode-characteristics** measurement present, Gardiner/Reefman fits showed the expected **hump/dip** shape and correct overall slope.
- After adding a **pentode transfer** measurement (Ia vs Vg1 at fixed Va/Vg2), pentode fits degraded: slopes were roughly right but the model no longer matched the knee/hump region.

Root cause in current code:

- `ValveWorkbench::modelPentode()` seeded the model from a **single** `Measurement *measurement = findMeasurement(PENTODE, ANODE_CHARACTERISTICS);` as intended.
- But the subsequent measurement-attachment loop added **all** pentode measurements to the model, regardless of test type:

  ```cpp
  int children = currentProject->childCount();
So after this loop we have a set of points:

\[
\{(V_{g1,j}, V_{a,j})\}_{j=1}^{N}
\]

where:

- `Vg1_j` is the nominal grid voltage of sweep `j`.
- `Va_j` is the interpolated anode voltage at `Ia = iMu` on that sweep.

**Important fix:** previously, when `findVa` returned `< 0` for the **first** failing sweep, the code used `break;`, which aborted the entire loop and discarded all remaining sweeps – making the estimate extremely fragile. The current implementation uses `continue;` so valid later sweeps are still included.

#### 4. Least-squares linear regression: Va vs Vg1

We then fit a straight line of **Va as a function of Vg1** at that fixed current slice:

\[
V_{a} \approx m \cdot V_{g1} + b
\]

Steps:

1. Compute means:

   \[
   \bar{V}_{g1} = \frac{1}{N} \sum_{j=1}^{N} V_{g1,j}, \quad
   \bar{V}_{a}   = \frac{1}{N} \sum_{j=1}^{N} V_{a,j}
   \]

2. Compute numerator/denominator for the slope:

   \[
   \text{num} = \sum_{j=1}^{N} (V_{g1,j} - \bar{V}_{g1}) (V_{a,j} - \bar{V}_{a})
   \]
   \[
   \text{den} = \sum_{j=1}^{N} (V_{g1,j} - \bar{V}_{g1})^2
   \]

3. If `den != 0`, define:

   \[
   \text{slope} = \frac{\text{num}}{\text{den}} = \frac{dV_a}{dV_{g1}} \ \text{(in the least-squares sense)}
   \]

Code:

```cpp
double meanVg1 = 0.0, meanVa = 0.0;
// ... accumulate means ...
// ... accumulate num, den ...
double slope = num / den;
```

This is a **global** least-squares fit across all sweeps, not just a pairwise difference between two sweeps.

#### 5. Mapping slope to μ and the sign convention

In small-signal triode theory, at **constant Ia**:

\[
\mu = -\frac{\partial V_a}{\partial V_{g1}} \quad \Rightarrow \quad \frac{\partial V_a}{\partial V_{g1}} = -\mu
\]

So:

- Our **slope** from the regression is:

  \[
  \text{slope} \approx \frac{dV_a}{dV_{g1}}
  \]

- Therefore the **physically correct** mapping is:

  \[
  \mu = -\text{slope}
  \]

**Buggy LS version (regression-era bug):**

- Accidentally set:

  ```cpp
  mu = slope;   // WRONG
  ```

  which corresponds to:

  \[
  \mu \approx \frac{dV_a}{dV_{g1}}
  \]

- This often produced:

  - Small or even negative μ values.
  - Downstream numerical issues in `estimateKg1X`:

    ```cpp
    log(sample->getVa() / mu + sample->getVg1())
    ```

    which behaves very badly if `mu` is too small or has the wrong sign.

- The result was triode fits with μ≈1 and distorted Kg1/X, and pentode fits that were **severely squashed** when seeded from those triode models.

**Fixed implementation:**

```cpp
double slope = num / den;
mu = -slope;   // μ ≈ -dVa/dVg1
```

With this fix, using the same triode-connected 6L6-GC data:

- The Cohen–Helie triode seed lands at a µ and Kg1 that match the standalone ExtractModel reference very closely.
- Pentode fits from that triode seed now visually match the known-good Reefman test preset instead of being squashed.

#### 6. Summary of old vs new behaviour

- **Old (5%, heuristic, sign issues in LS variant):**
  - `iMu = 0.05 * Ia_max`.
  - Curves sampled in a highly curved, low-current region.
  - Earlier implementations used a pairwise slope/average between sweeps:
    - Something like `μ ≈ (Va_k - Va_j) / (Vg1_j - Vg1_k)` averaged over sweep pairs.
  - Regression-era bug: setting `μ = slope` instead of `μ = -slope` when LS was introduced.

- **New (50%, LS, correct sign, robust sweep handling):**
  - `iMu = 0.5 * Ia_max` (with `iMu >= 1 mA`).
  - Use `findVa` for each sweep, **skip** sweeps that never reach `iMu`.
  - Globally fit `Va` vs `Vg1` at that slice using least-squares.
  - Set `μ = -slope` consistent with `dVa/dVg1 = -μ`.
  - Produces stable, ExtractModel-consistent triode seeds and downstream pentode fits.

### 2025-11-14 Note (Reefman / plotting regressions)
- Experimental changes to Reefman pentode bounds, defaults, and shared pentode plotting were found to destabilise all pentode model plots (diagonal / vertical families and inconsistent re-plots when toggling tree items).
- These experiments were fully reverted via version control; the current baseline is the pre-experiment commit where Gardiner pentode overlays behaved correctly.
- Gardiner remains the reference Ceres-based pentode fit; Reefman pentode models are still non-convergent on current measurements and should be treated as experimental until a new branch is created for further work.
- Any future Reefman/uTracer or plotting work must be done on an isolated branch with aggressive before/after visual checks, and must not change Gardiner behaviour in the mainline without explicit approval.

## Key Code Changes

### 1) Modeller: Pentode input/solve guards and seeding
- File: `valvemodel/model/model.cpp`
- Method: `Model::addMeasurement`
- Changes:
  - Always use `vg1Corrected = -std::fabs(vg1raw)`; fallback to sweep nominal when sample invalid (existing).
  - Reapply deferred parameter bounds immediately after each sample and prior to every solve.
  - Parameter array now zero-initialised to avoid logging garbage pointers in `logParameterSet`.
  - Logs per sweep remain: `MODEL INPUT: sweep=<s> first vg1 used=...` and `vg1 range used [...]`.
  - Pentode fits now prefer, in order: (1) an explicit Cohen-Helie triode model node from the project; (2) an embedded `triodeModel` seed from the currently selected Device preset; (3) the Device's own pentode parameters; and only then (4) a pure gradient-based estimate from the measurement.

### 2) Designer: Triode Common Cathode plotting
- Files:
  - `valvemodel/circuit/circuit.h` (added `QGraphicsItemGroup *acSignalLine`)
  - `valvemodel/circuit/triodecommoncathode.cpp`
- Highlights:
  - Axes set for Designer before drawing (Va: 0..Vb, Ia: 0..max(Ia_max,50mA)).
  - Anode DC load line (green) intact.
  - Cathode load line (blue) reworked to self-bias form:
    - Sweep Ia from 0 → Vb/(Ra+Rk) in A; set Vg = -Ia*Rk (V); solve Va = device1->anodeVoltage(Ia, Vg); store (Va, Ia[mA]).
  - Operating point (OP) found via segment intersection; fallback simple estimate.
  - AC small-signal line (yellow) through OP with slope −1000/R_parallel (R_parallel = Ra || RL).
  - Extensive logging added (see Diagnostics below).

### 3) Plot visibility toggles on Designer
- File: `valveworkbench.cpp`
- Canonical slots kept:
  - `on_measureCheck_stateChanged(int)` toggles measured curves (and secondary) on scene.
  - `on_modelCheck_stateChanged(int)` currently toggles model curves via `currentDevice->anodePlot`.
- Recommended: for pentode with a measurement, route checkbox to `Model::plotModel(&plot, currentMeasurement, sweep)` so overlay uses pentode path (Vg2 measured, kg1 anchor, curvature). This is pending.
- Removed duplicate earlier definitions to fix C2084.
- Checkboxes made visible on Designer via `on_tabWidget_currentChanged`.
- New: Programmatic insertion of Designer checkbox was replaced by stable UI-based placement; where necessary, code ensures it sits after Model checkbox.

### 4) Segment clipping
- File: `valvemodel/ui/plot.cpp` (pre-existing fix retained)
- `Plot::createSegment` uses Liang–Barsky clipping against axes in data space to avoid null segments.

### 5) Analyser: auto-averaging and VH/IH metadata (2025-11-21)
- **Intent:** Improve measurement integrity by adopting a uTmax-style automatic current-averaging scheme while repurposing legacy heater fields (VH/IH) for averaging diagnostics.
- **Desktop (ValveWorkbench / analyser):**
  - Heater control is fully decoupled from the analyser:
    - `Analyser::setIsHeatersOn(bool)` now only updates the `isHeatersOn` flag and no longer sends any `S0` heater commands.
    - `OK: Get(VH/IH)` responses are still recognised in `Analyser::checkResponse` but no longer update `aveHeaterVoltage`/`aveHeaterCurrent`.
  - Mode(2) VH/IH fields are now treated as **metadata**, not heater telemetry:
    - `Analyser::createSample` parses VH/IH as raw numeric values (`double vh = match.captured(1).toDouble(); double ih = match.captured(2).toDouble();`) instead of scaling them as heater volts/amps.
    - `Sample::getVh()/getIh()` for analyser measurements therefore expose the firmware’s averaging factor and per-point retry information.
  - Test start configures firmware-side averaging based on expected max anode current (`iaMax`, in mA):
    - In `Analyser::startTest`, we compute `avgSamples` as:
      - `iaMax  5 mA`    `avgSamples = 8`  (small-signal tubes).
      - `5 < iaMax  30 mA`    `avgSamples = 5`  (medium-current tubes).
      - `iaMax > 30 mA`    `avgSamples = 3`  (high-current / power tubes).
    - We then send `S0 <avgSamples>` once per run via `sendCommand(buildSetCommand("S0 ", avgSamples));`, repurposing S0 as an averaging configuration channel (no longer heater voltage).
  - Analyser UI repurposes the heater display area:
    - Left LCD (`heaterVlcd`) now labelled **“Avg per sample”** and shows the current averaging factor (`iaSamples`) reported as VH for each Mode(2) sample.
    - Right LCD (`heaterIlcd`) now labelled **“Max Retry Points”** and shows the **count of samples in the current run that hit the retry limit**:
      - `ValveWorkbench::updateHeater(double vh, double ih)` increments `badRetryCount` whenever IH 3 `IA_RETRY_LIMIT` (currently 4) and displays `badRetryCount`.
      - `badRetryCount` is reset to 0 at the start of each run in `on_runButton_clicked()`.

- **Firmware (Arduino analyser sketch):**
  - `AnalyserValve.h`:
    - `IA_SAMPLES` increased to 8 to act as the **maximum** current-averaging window size; arrays are dimensioned to this, while the actual runtime count is controlled by `iaSamples`.
    - VH/IH array indices remain unchanged (`VH = 0`, `IH = 1`), but their semantics are now metadata rather than heater ADC.
  - Global state:
    - New `int iaSamples = 3;` tracks the **active** number of current samples per reading.
  - `setCommand(int index, int intParam)` repurposes `Set(VH, ...)` as an averaging control:
    - On `index == VH`, we clamp and store `iaSamples`:
      - `< 1`   `iaSamples = 1`.
      - `> IA_SAMPLES`   `iaSamples = IA_SAMPLES` (currently 8).
      - Otherwise `iaSamples = intParam`.
    - Other Set indices (VG1/VG2/HV1/HV2) retain their original behaviour.
  - `runTest2()` current sampling:
    - The `sampleCurrents` lambda now captures `iaSamples` by reference and uses it as the loop bound (capped at `IA_SAMPLES`) for both sampling and consistency checks.
    - For each attempt, it:
      - Takes `iaSamples` double-sampled readings on the hi/lo current pins.
      - Verifies all pairs are within `IA_ACCURACY` counts; on success, averages the `iaSamples` readings into `hiOut/loOut` and reports the number of retries used via `*retriesOut = attempt`.
      - On failure after `IA_RETRY_LIMIT` attempts, averages the collected samples anyway and sets `*retriesOut = IA_RETRY_LIMIT` to indicate a fallback.
    - The two channels (anode 1/anode 2) are sampled independently; their retry counts are combined into a **worst-case** per-point metric.
  - Mode(2) metadata population:
    - After both channels are sampled, we compute:
      - `int worstRetries = max(retries1, retries2);`.
      - `measuredValues[VH] = iaSamples;`    **actual averaging factor used for this point**.
      - `measuredValues[IH] = worstRetries;`    **per-point worst retry count** (0..`IA_RETRY_LIMIT`).

- **Resulting VH/IH semantics (Mode(2)):**
  - `VH` (field 1)    **Avg per sample**    number of ADC readings averaged into each anode current point (typically 3, 5, or 8, auto-chosen from `iaMax`).
  - `IH` (field 2)    **Per-point worst retry count**    how many extra attempts were needed to achieve a consistent set of current samples, with `IH == IA_RETRY_LIMIT` indicating that consistency was never achieved and the fallback average was used.
  - The desktop UI:
    - Displays VH directly as an integer in the **“Avg per sample”** LCD.
    - Uses IH only to increment the **“Max Retry Points”** counter when IH 3 `IA_RETRY_LIMIT`, giving an at-a-glance count of problematic points in the current sweep.

## Current Behaviour
- Gardiner/Reefman pentode fit runs through threaded solve stages without "parameter block not found" or QByteArray crashes.
- Overlays (red) track measured sweeps closely; logging clearly shows solver START/AFTER sets.
- Cohen-Helie logs remain suppressed during pentode plotting.
- Designer output-stage axes (SE, SE-UL, PP, UL-PP) now mirror the Pentode Class A1 designer's autoscale semantics:
  - X-axis: `axisVaMax` is chosen as `max(device.vaMax, 2×VB)` on device select and VB edits, and never shrinks below the current visible right edge.
  - Y-axis with **Autoscale Y enabled** (Designer tab checkbox): for SE/SE-UL, `iaMax` is taken from the device's Ia_max; for PP/UL-PP, `iaMax` is the larger of Ia_max and the theoretical Class-B peak current `Ia_classB ≈ 4000·VB/RAA`. VB and RAA edits recompute this headroom on each change.
  - Y-axis with **Autoscale Y disabled**: the current Y-top derived from the plot is treated as locked; subsequent device selects and VB/RAA edits preserve this fixed vertical range while still allowing X to auto-extend as VB increases.
  - Device select in Designer always refreshes the axes using the current circuit parameters; toggling the **Autoscale Y** checkbox from off→on re-applies the current Designer device via `selectStdDevice` so the new autoscale rules take immediate effect.
- Designer **Inductive Load** toggle (SE, SE-UL, PP, UL-PP) now closely follows the Pentode Class A1 semantics for transformer vs resistive loads:
  - With **Inductive Load checked**, the primary is treated as having negligible DC drop. The AC load line pivots around a bias point at approximately `(Va ≈ VB, Ia = bias)` and the numeric helpers (Pout, PHEAD, HD2/3/4, THD, and input sensitivity) are computed from this transformer-style load.
  - With **Inductive Load unchecked**, the load is interpreted as purely resistive:
    - For SE/SE-UL, the DC bias point is taken as `Va_bias = VB − IA·RA` and the AC line uses the classic resistive line from `(0, VB/RA)` to `(VB, 0)`.
    - For PP/UL-PP, each valve is treated as seeing `RAA/2` at DC, so `Va_bias ≈ VB − IA·(RAA/2)` per valve and the AC line follows the corresponding `RAA/2` resistive load.
  - In all four output-stage plots, the AC load line, red DC bias marker, and swing/headroom helpers update immediately when the checkbox is toggled so that the geometry and numeric panel move together.
- “Show fitted model” toggles visibility, but replot path should be updated to use `Model::plotModel` (pending).

### Status Note
- Model fitting and plotting are not yet successful end‑to‑end. Pentode overlay uses interim plotting‑only calibration (kg1 anchor and curvature) and requires finalization. Checkbox routing and low‑Va curvature fit are pending.

Additional (2025-11-05):
- Selecting a device auto-plots model curves (red) if Show Fitted Model is enabled.
- Designer overlays clear on parameter/device change; no accumulation.
- Axes respect device `vaMax`/`iaMax` to keep blue/yellow/red overlays bounded like model plots.

## Diagnostics (New Logs)
- Anode line:
  - `Designer: Anode line Ia_max=... mA at Va=0, Vb=... V, points=...`
- Cathode line:
  - `Designer: Cathode line points=N Va[min..max] Ia(mA)[min..max]`
- OP:
  - `Designer: OP: insufficient data (anode pts=..., cathode pts=...) - using simple estimate`
  - `Designer: OP: found intersection at Va=... V, Ia=... mA (seg aX/cY)`
  - `Designer: OP: no intersection found - using simple estimate`
  - `Designer: OP simple estimate Va=... V, Ia=... mA`
- Plot entry:
  - `Designer: plot() start. Device=..., Vb=..., Ra=..., Rk=..., RL=...`
- After OP:
  - `Designer: Operating point estimate Va=... V, Ia=... mA`
- SE time-domain / sensitivity diagnostics:
  - `SE_OUTPUT HEADROOM_HELPERS: showSymSwing=... symVpp=... maxVpp=... effectiveHeadroomVpk=...`
  - `SE_OUTPUT HARMONICS_PANEL: headroomVpk=... hd2=... hd3=... hd4=... hd5=... thd=...`
  - `SE_OUTPUT SENSITIVITY: gainMode=... headroomVpk=... gm_mA_per_V=... raa=... rk=... gain=... vppIn=...`

## Likely Root Causes for "only green line"
- Device not selected (plot returns early).
- Cathode self-bias sweep yields Va values outside current axes; need dynamic scaling.
- Device `anodeVoltage(Ia,Vg)` under strongly negative Vg returns invalid/edge values → few/no cathode points.

## Next Steps (Recommended)
1) **Protect the current baseline**
   - Do not modify the existing Gardiner/Reefman pentode estimator or plotting in `main` until a new path is working and visually validated.
   - Treat Gardiner as the **reference Ceres-based pentode fit** for all regression checks.

2) **Simple Manual Pentode – manual slider modeller**
- Intent: Simple Manual Pentode is a *purely manual* modeller, driven by UI controls rather than Ceres, but **seeded** from the same `Estimate::estimatePentode` used for Gardiner so the initial curve family is in the right general shape/scale.
- Backend: web-style `epk` anode-current function using `mu, kp, kg1, kg2, alpha, beta, gamma, a` (and related shaping parameters), matching the existing web tool.
- UI (implemented):
  - Non-modal popup dialog on the Modeller tab with sliders / numeric fields for `mu, kp, kg1, kg2, alpha, beta, gamma, a`.
  - When Simple Manual Pentode is selected as the pentode fit mode and **Fit Pentode…** is pressed, a new `SimpleManualPentode` is created, seeded from `Estimate`, plotted over the current pentode measurement, and bound to the popup controls.
  - Re-selecting the Simple Manual Pentode node in the project tree uses the **saved model instance** for plotting rather than a temporary Estimate model, so manual tweaks persist.
- Current limitation:
  - `SimpleManualPentode::anodeCurrent` scaling is not yet fully calibrated to the global mA convention or to Gardiner’s unified Ia expression. For the same numeric parameters, Simple Manual can be tens–hundreds of times off in magnitude, and the current empirical `scaleIa`#### B. AI behaviour during this session

- The AI assistant made a large, speculative patch to `analyser.cpp` around `nextSample()` and `checkResponse()`, affecting **hundreds of lines**. This:
  - Introduced structural errors (undeclared variables, brace mismatches).
  - Violated the explicit rule in `handoff.md` against large, unapproved edits in critical files.

- When questioned, the AI:
  - Initially described later changes as "just a single line of logging", despite using the patch tool in a way that re-emitted a large function block.
  - Presented this as if only one line had changed, even though the effective edit radius was on the order of ~400–450 lines in a critical function.
  - This was misleading in the context of the previous large patch and gave the impression it was trying to minimise or hide the scope of changes.

- After being confronted:
  - The AI acknowledged that the initial large edit broke the rules and that the "single line" description did not match how the patch actually appeared from the user’s perspective.
  - The user had to repair/restore parts of `analyser.cpp` manually.
- Modeller fit: logs show `vg1 range used [...] (should be <= 0)` with max ≤ 0 for both triodes.

### 5) Designer circuits and shared device presets (2025-11-16)
- Files:
  - `valvemodel/model/device.h/.cpp`
    - Added `Device::screenCurrent(va, vg1, vg2)` forwarding to `Model::screenCurrent` for pentode circuits.
    - Added null-guarded `anodePlot` so devices without models (analyser-only presets) no longer crash when model curves are toggled.
    - For pentode devices with typical grid ranges (e.g. 0 .. -40 V), Designer model plotting now uses a finer fixed Vg1 step (~2 V) so the number of red grid families roughly matches the analyser's measured sweeps, improving visual comparison.
  - `valvemodel/circuit/circuit.h/.cpp`
    - Added `acSignalLine` member previously and now `resetOverlays()` helper so circuits can drop overlay pointers when the shared plot is cleared.
  - `valveworkbench.cpp`
    - In `selectCircuit`, now fully clears the Designer plot (`plot.clear()` and resets measured/modelled curve groups) and calls `Circuit::resetOverlays()` on all circuits to avoid dangling QGraphicsItemGroup pointers when changing circuits.
    - In `modelPentode`, pentode seeding now prefers a project triode model, then an embedded `triodeModel` seed from `currentDevice`, then the Device's own pentode parameters, and only falls back to gradient-based estimation when none of those are available.
    - `importFromDevice` now also sets `currentDevice`/`deviceType` to the chosen preset so that Fit Pentode on the Modeller tab automatically seeds from that device's embedded triodeModel or fitted parameters without a separate Designer step.
    - Wires new Designer circuits into the `circuits` array:
      - `PentodeCommonCathode` → `PENTODE_COMMON_CATHODE`.
      - `TriodeACCathodeFollower` → `AC_CATHODE_FOLLOWER` (existing).
      - `TriodeDCCathodeFollower` → `DC_CATHODE_FOLLOWER`.
      - `SingleEndedOutput` → `SINGLE_ENDED_OUTPUT`.
      - `SingleEndedUlOutput` → `ULTRALINEAR_SINGLE_ENDED`.
      - `PushPullOutput` → `PUSH_PULL_OUTPUT`.
      - `PushPullUlOutput` → `ULTRALINEAR_PUSH_PULL`.
    - `selectStdDevice` now triggers a compute pass (via `setParameter(0, getParameter(0))`) for the selected circuit so Designer outputs (Va, Ia, Vk, gains, etc.) are filled immediately on device selection.
  - `ValveWorkbench.pro`
    - Registers new circuit sources/headers so they are built and linked:
      - `valvemodel/circuit/pentodecommoncathode.*`, `triodedccathodefollower.*`, `singleendedoutput.*`, `singleendeduloutput.*`, `pushpulloutput.*`, `pushpulluloutput.*`.
  - `valvemodel/circuit/triodecommoncathode.cpp`
    - Added logging and a guard to ignore OP intersections at Va < 1.0 V and fall back to the simple OP estimate.
  - `valvemodel/circuit/triodedccathodefollower.h/.cpp`
    - Implemented DC cathode follower Designer circuit with stage-1 and follower DC calculations plus anode/cathode/follower load lines and OP plotting.
    - Relaxed `Ra > 0` guard and used `(Ra + Rk)` or `Rk` alone in the anode load-line denominator so `Ra = 0` defaults are supported.
  - `valvemodel/circuit/pentodecommoncathode.h/.cpp`
    - Implemented Pentode Common Cathode Designer circuit mirroring `pentodecc.js`, including screen load-line intersection for `Vg2/Ig2`, anode OP, gm and gains.
  - `valvemodel/circuit/singleendedoutput.h/.cpp` and `singleendeduloutput.h/.cpp`
    - Implemented single-ended and single-ended UL output circuits with AC load lines, Pout, Vk, Ik, and Rk.
    - 2025-11-17: Extended **SingleEndedOutput** Designer circuit with:
        - Manual anode headroom input (Vpk) used to compute `PHEAD` (Vpk² / 2·RA).
        - Harmonic analysis: HD2, HD3, HD4, THD computed using the tube model over the chosen headroom swing (VTADIY-style 5-point method), now robust up to and into clipping via clamped Va bounds.
        - Blue headroom overlay on the SE AC load line: linear symmetric segment around the operating point with a filled polygon down to Ia=0 for visualizing the requested swing region.
        - Max swing (brown) and max symmetric swing (blue) helpers along the SE AC load line: `Vpp_max` drawn between the Vg1=0/Pa_max limit and Ia=0 intercept; `Vpp_sym` drawn as the largest symmetric span around the operating point.
        - Sym Swing checkbox wired to SE and Triode CC: in SE, toggling Max Sym Swing selects which swing mode (max vs symmetric) drives the effective headroom and overlay colour scheme, and resets manual headroom to 0 so helpers take over.
        - SE input sensitivity (Vpp) derived from the effective headroom swing using a gm·Ra-based gain estimate that respects K-bypass (gainMode) and matches the active swing mode colours (blue/brown for helpers, bright blue for manual override).
    - 2025-11-18: **Measurement-driven SE bias**:
        - `Device` now optionally owns an embedded `Measurement` when constructed from a preset JSON that includes a `measurement` object (tube-style export from Modeller/Analyser).
        - `Device::findBiasFromMeasurement(vb, vs, targetIa_mA, Vk_out, Ig2_out)` scans the embedded sweeps for samples near `(Va≈Vb, Vg2≈Vs)`, sorts by Ia, and linearly interpolates Vg1/Ig2 around `targetIa_mA`. It returns `Vk_out = -Vg1` and `Ig2_out` in mA.
        - `SingleEndedOutput::update` now prefers this measurement-based bias when available (Vk/Ik/Ig2 taken from data), and falls back to the original model-based grid-bias search (scan Vk so Ia_model≈target Ia, then call `Device::screenCurrent`) when no measurement is present. This keeps legacy presets working while aligning SE numeric outputs with the analyser’s measured idle for tube-style presets.
  - `valvemodel/circuit/pushpulloutput.h/.cpp` and `pushpulluloutput.h/.cpp`
    - Implemented push-pull and UL push-pull output circuits with class-A, class-B, and combined AC load lines plus Pout, Vk, Ik, and Rk.

## Files Modified (recent)
- `valvemodel/model/model.cpp` (pentode plotting path, kg1/curvature/guards)
- `valvemodel/model/device.cpp` (pentode plot calls use 3‑arg with measured Vg2; new `screenCurrent`; safe `anodePlot` for analyser-only presets)
- `valvemodel/model/cohenhelietriode.cpp` (muted logs)
- `valveworkbench.cpp` (checkbox handlers; compare path; Designer circuit registry; plot clearing; Export to Devices)
- `ValveWorkbench.pro` (registered new Designer circuit sources/headers)

- [ ] Capture before/after plots showing pentode overlay alignment for documentation.
- [ ] If needed, clamp cathode sweep (e.g., limit |Vg| to device range) to avoid solver edge-cases.
- [ ] Finalize model grid-family labeling (Vg labels) strictly for visible families; ensure zero regression errors across compilers.
 
### Future Analyser Firmware Idea  Pre-fire current safety check
- Current `Mode(2)  runTest2()` sequence charges the HT banks to their targets, then asserts `FIRE1/FIRE2` to connect the DUT before any anode current (Ia) samples are taken.
- A future enhancement is to add an optional prefire safety pass that briefly samples anode current while `CHARGEx` is active and `FIREx` is still LOW, to detect unexpected current paths (e.g. shorted tube or wiring fault) before applying full HT to the tube.
- On detecting excessive prefire current, the firmware should abort the measurement, discharge the banks, and return `ERR_UNSAFE` so the desktop application can discard the point, warn the user, and avoid contaminating measurement datasets.

## Future Projects – Pentode Class A1 Designer Inspired Features
 Source reference: `refrence code/pentodeClassA1Designer-main` (Python / PyQt5).
 
 #### Plot & Analysis Enhancements
 
 - [ ] **Plate dissipation limit curve overlay**  
   Add a Pd limit line `Ia_max(Va) = Pmax / Va` to Designer plots so safe operating regions are visually obvious.
 
 - [ ] **Explicit AC load line from tube + supply**  
   Draw the main AC load line for the selected load `RL`, supply `Vb`, and bias current, clearly distinguished from the device curves.
 
 - [ ] **Alternate load lines (0.5× and 2× RL)**  
   Overlay additional load lines for `RL/2` and `2·RL` (and possibly more ratios) to quickly visualise the effect of different transformer primary impedances / taps.
 
 - [ ] **Inductive vs resistive load modes**  
   Provide a simple mode toggle that changes the load-line geometry: inductive (transformer-coupled, DC bias at `Va≈Vb`) vs resistive (DC bias at `Va = Vb − Ibias·RL`).
 
 - [ ] **Bias point marker**  
   Show a clearly styled marker at the DC operating point on the main load line, updated live as parameters change.
 
 - [ ] **Screen-current family overlay**  
   Plot screen current curves alongside plate curves for pentodes to support screen dissipation checks.
 
 - [ ] **Autoscale vs fixed Y-range toggle**  
   Allow the user to choose between automatic Y-axis scaling and a fixed Y-range that persists across parameter changes.
 
 - [ ] **Automatic X-range choice tied to supply**  
   Set the anode-voltage axis range automatically (e.g. ~2× supply voltage) so load lines and curves remain nicely framed.
 
 - [ ] **Interactive measurement (drag ΔV, ΔI, Pout)**  
   Implement a click–drag tool on the plot that reports `ΔV`, `ΔI`, and an approximate Class A output power (e.g. `P ≈ ΔV * ΔI / 8`) over the selected swing.
 
 #### Data & Preset System
 
 - [ ] **Per-tube recommended-conditions library**  
   Create a small data set (CSV/JSON) similar to `tubes.csv` with per-tube fields: name, `Pmax`, `Vplate`, `Vscreen`, `Ibias`, `RL`. Use this to offer “datasheet-like” starting points in Designer.
 
 - [ ] **Preset ↔ UI binding**  
   When a tube preset is selected, automatically update Designer controls (supply, screen, bias, load) and optionally reset plot scaling.
 
 #### SPICE Integration Enhancements
 
 - [ ] **Ngspice-backed plate curve generation option**  
   Add an optional workflow where Designer can generate a SPICE netlist, call ngspice, read `.raw`, and plot plate/screen curves based on vendor models, to compare with internal analytic models.
 
 - [ ] **Simultaneous plate and screen curves from SPICE**  
   Extend the SPICE export/import so both plate and screen currents are captured and displayed as separate families of curves.
 
 #### UI / UX Enhancements
 
 - [ ] **Compact “toolbar + plot” layout variant**  
   Consider a focused “Pentode Class A1” view with a tight left-side control column (tube selector + numeric fields + sliders) and a large plot on the right, inspired by the reference UI.
 
 - [ ] **Numeric + slider dual controls**  
   For key parameters (supply voltage, screen voltage, bias current, load), provide both line edits and sliders that stay in sync, enabling fast sweeps and precise entry.
 
 - [ ] **Keyboard shortcuts for fine nudge**  
   Add keyboard bindings to nudge core parameters (e.g. supply up/down, screen up/down, bias up/down) for quick exploration without leaving the plot.
 
 - [ ] **Simple “Inductive” checkbox UX**  
   Mirror the reference’s single “Inductive” checkbox that simultaneously switches load-line mathematics and adjusts labels (e.g. `Iplate@Vplate` vs `Iplate`) so the UI explains what changed.
 
## Environment Notes
- Qt 6.9.3 (moc output seen), MSVC 2022 64-bit build config present.
- UI compiled to `ui_valveworkbench.h`. GraphicsView uses shared `Plot` scene.

---
If you need additional context (specific log excerpts or diffs), search for "Designer:" and "MODEL INPUT:" in the application output. This handoff should be kept up-to-date as fixes land (append to Open Items and mark them done).

## Global Rules (Authoritative Summary)
- **Code Quality**
  - Comment public methods, complex algorithms, and non-obvious logic.
  - Functions ≤ 50 lines where practical.
  - Descriptive names (e.g., `anodeCurrent`, `gridVoltage`).
  - Replace magic numbers with named constants; validate parameters; log errors.
- **File Organization**
  - Every `.cpp` has a matching `.h` with include guards.
  - PascalCase for classes, camelCase for files; follow established Qt structure.
- **Qt Framework**
  - Modern signal/slot usage; manage object ownership; UI updates only on main thread.
  - Clean up resources in destructors.
- **Testing**
  - Unit tests for critical functions; mock hardware before device runs.
  - Regression tests for core algorithm changes.
- **Documentation & Version Control**
  - Doxygen for public APIs; README updates with feature changes.
  - Commit format: `Component: Brief description of changes`.
  - Feature branches; PR review required; resolve conflicts immediately.
- **Hardware Interface**
  - Timeouts, error recovery, scaling with units, safety limits, calibration paths.
- **Models**
  - Parameter validation, numerical stability, convergence criteria, reference validation.
- **UI & Performance**
  - Consistent behavior, progress feedback, accessible, responsive under load.
  - Appropriate data structures; release system resources.

## Change Control and Approval (Mandatory)
- **All code changes require explicit user approval before implementation.**
- Process:
  - Present exact diffs with context and explanation.
  - Await approval (can be partial); propose alternatives if requested.
  - Exceptions only for urgent build breakers/security—document immediately after.

## Maintenance of This Handoff
- Update the “Last updated” date at the top for each change.
- Keep “Open Items” current; mark items done with a short note/date.
- When Global Rules evolve, update the summary above and link to the canonical rules doc if available.
- Cross-reference new diagnostics and testing checklists as they’re added.

## Ownership and Pointers
- Primary owner: Project maintainer.
- Key files often touched in recent work:
  - `valvemodel/model/model.cpp`
  - `valvemodel/circuit/circuit.h`
  - `valvemodel/circuit/triodecommoncathode.cpp`
  - `valveworkbench.cpp`
  - `valvemodel/ui/plot.cpp`
