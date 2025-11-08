# ValveWorkbench - Engineering Handoff

Last updated: 2025-11-07

## Project Snapshot
- Qt/C++ vacuum tube modelling and circuit design app (Designer, Modeller, Analyser tabs)
- Plot system: custom `Plot` with QGraphicsScene. Curves grouped via `QGraphicsItemGroup`.
- Models: Cohen-Helie Triode et al. Solver: Ceres.

## Recent Focus
- Designer plotting improvements (anode/cathode load lines, OP, AC small-signal line).
- Visibility toggles for measured/modelled plots on Designer.
- Ensure model fitting always uses non-positive grid voltages (single and double triode).

### 2025-11-05 Summary
- Added an Export to Devices button on Modeller to save the fitted model JSON to `models/` and refresh Designer device dropdowns.
- Added a Designer overlays checkbox (Show Designer Overlays) alongside existing toggles; manages anode/cathode/AC/OP overlays without affecting measurement/model plots.
- Auto-plot model curves in Designer when a device is selected (if Show Fitted Model is checked) so red model families appear without a measurement.
- Clamped Designer axes to device limits (`vaMax`, `iaMax`) so overlay lines do not exceed model plot extent.
- Ensured Designer overlays are cleared before replot (no stacking) and do not interfere with measurement/model visibility.
- gm vs bypass semantics: gm is an intrinsic device parameter at OP and does not change with bypass; cathode bypass only removes local degeneration so stage gain increases (effective gm toward gm as Zk_ac→0).

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
  - `on_modelCheck_stateChanged(int)` toggles model curves (`modelPlot`, modelled/estimated groups).
- Removed duplicate earlier definitions to fix C2084.
- Checkboxes made visible on Designer via `on_tabWidget_currentChanged`.
- New: Programmatic insertion of Designer checkbox was replaced by stable UI-based placement; where necessary, code ensures it sits after Model checkbox.

### 4) Segment clipping
- File: `valvemodel/ui/plot.cpp` (pre-existing fix retained)
- `Plot::createSegment` uses Liang–Barsky clipping against axes in data space to avoid null segments.

## Current Behaviour
- Green anode line renders.
- Designer checkboxes (Measured, Model) visible and functional (toggling overlays).
- Some user runs still show only green line; new diagnostics added to pinpoint cause for missing blue/OP/AC.

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
1) Add a "Calculate" button (Designer) to explicitly recompute and draw without relying on editingFinished.
   - UI: Add QPushButton under the 16 parameter lines (Designer tab).
   - Slot `on_calculateButton_clicked()`:
     - Get current circuit: `auto cType = ui->circuitSelection->currentData().toInt();`
     - `circuits.at(cType)->plot(&plot);`
     - `circuits.at(cType)->updateUI(circuitLabels, circuitValues);`

2) Improve Designer axis scaling for cathode line:
   - After generating cathode points, compute min/max Ia and Va and expand axes accordingly (e.g., yStop = max(yStop, maxIa+margin)).

3) Validate device solver call for cathode sweep:
   - Confirm `device1->anodeVoltage(Ia, Vg)` expects Vg relative to ground (negative for bias).
   - If solver expects different sign/reference, adjust mapping (e.g., ensure `Vg <= 0`).

4) Confirm checkbox handling is consistent across tabs:
   - Keep only one implementation of measure/model slot handlers.

5) Verify Modeller negative-grid logs in single and double triode runs (max of range must be ≤ 0).

## Test Checklist
- Designer: Triode CC, Device: 12AX7, Params: Vb=250, Ra=100k, Rk=1.5k, RL=1M.
- Verify logs for anode/cathode/OP messages.
- Blue cathode line visible; OP (red dot) drawn; yellow AC line centered at OP.
- Toggle overlays via Measured/Model checkboxes.
- Modeller fit: logs show `vg1 range used [...] (should be <= 0)` with max ≤ 0 for both triodes.

## Files Modified (recent)
- `valvemodel/model/model.cpp`
- `valvemodel/circuit/circuit.h`
- `valvemodel/circuit/triodecommoncathode.cpp`
- `valveworkbench.cpp`

## Open Items
- [x] Add "Calculate" button to Designer and wire to recompute/plot. (Cleared per user request on 2025-11-07)
- [x] Axis auto-scaling incorporating cathode line ranges. (Cleared per user request on 2025-11-07)
- [x] If needed, clamp cathode sweep (e.g., limit |Vg| to device range) to avoid solver edge-cases. (Cleared per user request on 2025-11-07)
- [x] Validate UI update pathway to ensure OP/gain outputs refresh on Calculate. (Cleared per user request on 2025-11-07)
- [x] Finalize model grid-family labeling (Vg labels) strictly for visible families; ensure zero regression errors across compilers. (Cleared per user request on 2025-11-07)

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
