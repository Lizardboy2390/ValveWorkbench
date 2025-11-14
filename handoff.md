# ValveWorkbench - Engineering Handoff

Last updated: 2025-11-13 (end of pentode fitting experiments)

## Project Snapshot
- Qt/C++ vacuum tube modelling and circuit design app (Designer, Modeller, Analyser tabs)
- Plot system: custom `Plot` with QGraphicsScene. Curves grouped via `QGraphicsItemGroup`.
- Models: Cohen-Helie Triode et al. Solver: Ceres.

## Recent Focus
- Pentode plotting and model overlay correctness for GardinerPentode.
- Remove confusing Cohen-Helie logs during pentode plotting.
- Ensure Vg1 families for plotting come directly from measurement (−20…−60 V).
- Restore analyser to baseline regarding first-sample and limit-clamp (measurement integrity).

### 2025-11-13 Summary (Pentode)
- Multiple experiments were run to improve Gardiner/Reefman pentode fitting (kg1 calibration, plotting‑only A/beta/gamma, estimator tweaks, SimplePentode prototype).
- These changes caused unstable or non‑physical behaviour (vertical lines, extreme Ia, wrong slopes), so **all experimental pentode changes were reverted**; code is back to the original baseline.
- Triode modelling remains solid; pentode modelling is usable but still flatter than desired.

## Key Code Changes

### 1) Modeller: Force negative grid to solver
- File: `valvemodel/model/model.cpp`
- Method: `Model::addMeasurement`
- Change: Always use `vg1Corrected = -std::fabs(vg1raw)`; fallback to sweep nominal when sample invalid.
- Added logs per sweep:
  - `MODEL INPUT: sweep=<s> first vg1 used=...`
  - `MODEL INPUT: sweep=<s> vg1 range used [min, max] (should be <= 0)`

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
- Red pentode model overlays now appear with correct families; scale aligns at −20 V; curvature present.
- Cohen‑Helie logs suppressed during pentode plotting.
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
1) **Do not modify the existing Gardiner/Reefman pentode estimator or plotting until a new path is working.** Baseline behaviour is restored and should be treated as reference.
2) Implement a new **Simple Manual Pentode** model that mirrors the web `pentodemodeller.js` equations:
   - Backend: web-style `epk` and anode-current function using `mu, kp, kg1, kg2, alpha, beta, gamma, a`, no Ceres auto-fit initially.
   - UI: sliders / numeric fields on the existing Modeller pentode tab for these parameters, replotting curves live over measured pentode data.
   - Selection: add "Simple Manual Pentode" as a model type alongside Gardiner/Reefman in the existing pentode model selection.
3) Once the Simple Manual Pentode behaves like the web modeller for representative tubes (e.g. 6L6, EL34), design a **minimal auto-fit layer** on top of it (fit only a small subset of parameters) and wire it in as an optional step.
4) After Simple Manual Pentode is stable, reassess whether Gardiner/Reefman pentode fitting needs further work or can be left as-is.

## Test Checklist
- Designer: Triode CC, Device: 12AX7, Params: Vb=250, Ra=100k, Rk=1.5k, RL=1M.
- Verify logs for anode/cathode/OP messages.
- Blue cathode line visible; OP (red dot) drawn; yellow AC line centered at OP.
- Toggle overlays via Measured/Model checkboxes.
- Modeller fit: logs show `vg1 range used [...] (should be <= 0)` with max ≤ 0 for both triodes.

## Files Modified (recent)
- `valvemodel/model/model.cpp` (pentode plotting path, kg1/curvature/guards)
- `valvemodel/model/device.cpp` (pentode plot calls use 3‑arg with measured Vg2)
- `valvemodel/model/cohenhelietriode.cpp` (muted logs)
- `valveworkbench.cpp` (checkbox handlers; compare path)

## Open Items
- [ ] Add "Calculate" button to Designer and wire to recompute/plot.
- [ ] Axis auto-scaling incorporating cathode line ranges.
- [ ] If needed, clamp cathode sweep (e.g., limit |Vg| to device range) to avoid solver edge-cases.
- [ ] Validate UI update pathway to ensure OP/gain outputs refresh on Calculate.
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
