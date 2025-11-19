# ValveWorkbench - Engineering Handoff

Last updated: 2025-11-18 (Tube-style device presets with embedded measurement/triodeModel; Designer SE bias from measurement; shared presets still honoured)

## Project Snapshot
- Qt/C++ vacuum tube modelling and circuit design app (Designer, Modeller, Analyser tabs)
- Plot system: custom `Plot` with QGraphicsScene. Curves grouped via `QGraphicsItemGroup`.
- Models: Cohen-Helie Triode et al. Solver: Ceres.

## Recent Focus
- Pentode plotting and model overlay correctness for GardinerPentode.
- Remove confusing Cohen-Helie logs during pentode plotting.
- Ensure Vg1 families for plotting come directly from measurement (−20…−60 V).
- Restore analyser to baseline regarding first-sample and limit-clamp (measurement integrity).
 - Add a dedicated **Triode-Connected Pentode** analyser mode to generate triode-like sweeps for pentode tubes and feed UTmax-style seeding.
 - Centralise Gardiner vs Reefman pentode parameter bounds in `Model::setEstimate` so fits stay inside model-appropriate corridors.

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

## Current Behaviour
- Gardiner/Reefman pentode fit runs through threaded solve stages without "parameter block not found" or QByteArray crashes.
- Overlays (red) track measured sweeps closely; logging clearly shows solver START/AFTER sets.
- Cohen-Helie logs remain suppressed during pentode plotting.
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
  - `SimpleManualPentode::anodeCurrent` scaling is not yet fully calibrated to the global mA convention or to Gardiner’s unified Ia expression. For the same numeric parameters, Simple Manual can be tens–hundreds of times off in magnitude, and the current empirical `scaleIa` factor is only a stopgap. Manual curves are therefore not yet production-quality; Gardiner remains the reference fit.
- Behavioural contract (target):
  - No Ceres fitting in the manual path; the user is in full control of parameter values once seeded.
  - No shared mutable state with Gardiner/Reefman fitting beyond reading the same Measurement; changing sliders must not affect Gardiner in any way.

3) **Reefman Pentode – alignment with ExtractModel_3p0 (experimental branch only)**
   - Goal: Bring the `ReefmanPentode` model’s static Ia/Ig2 behaviour closer to Derk Reefman’s ExtractModel tool located at `C:\Users\lizar\Documents\ExtractModel_3p0` for representative tubes.
   - Process (high level):
     - Create a dedicated branch (e.g. `feature/reefman-extractmodel`) and keep all Reefman/uTracer work isolated there.
     - For a chosen tube, take a trusted parameter set from ExtractModel_3p0 and record Ia/Ig2 values at a small grid of (Va, Vg1, Vg2) points.
     - In that branch, compare `ReefmanPentode::anodeCurrent` (and screen-current path) against ExtractModel outputs at those points:
       - Confirm unit conventions (mA vs A, kg1/kg2 scaling, exponents alpha/beta/gamma, any clipping or offsets).
       - Adjust the Reefman equations only in that branch until Ia/Ig2 match ExtractModel within a small tolerance for the test grid, *before* enabling Ceres fitting.
     - Once static behaviour matches, carefully reintroduce Ceres bounds and seeding for Reefman, still on the experimental branch, and verify that Gardiner behaviour is unchanged.
   - Mainline policy:
     - No Reefman/uTracer changes should be merged into `main` unless Gardiner plots are visually unchanged and regression plots pass.
     - ExtractModel alignment is a long‑running, experimental effort and should not disrupt day-to-day use of Gardiner or Simple Manual Pentode.

4) **Reevaluation after manual path is solid**
   - Once the Simple Manual Pentode path behaves like the web modeller for representative tubes (e.g. 6L6, EL34), revisit Reefman fitting to decide whether it should be maintained, replaced by the manual+minimal-fit path, or left as an advanced/experimental option only.

## Test Checklist
- Designer: Triode CC, Device: 12AX7, Params: Vb=250, Ra=100k, Rk=1.5k, RL=1M.
- Verify logs for anode/cathode/OP messages.
- Blue cathode line visible; OP (red dot) drawn; yellow AC line centered at OP.
- Toggle overlays via Measured/Model checkboxes.
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
