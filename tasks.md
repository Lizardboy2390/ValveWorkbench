# Valve Workbench — Tasks & Change Log (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Active tasks (end-user focused)
- [ ] Designer: ensure device selection auto-replots and axes clamp to device limits.
- [ ] README / docs: clearly describe that Gardiner is the stable reference pentode fit in `main`, that **Simple Manual Pentode** is the manual slider-based path, and that Reefman/uTracer/ExtractModel work belongs on an experimental branch.
 - [ ] Integrated SPICE export via Devices:
   - Extend **Export model to Device** so each device JSON optionally includes a SPICE representation of the fitted tube model (triode and pentode) suitable for `.inc`/`.subckt` use.
   - Implement **File → Export to Spice...** so it uses the active Device's SPICE block to export (a) tube model only (`.inc`/`.lib`), and (b) optional Designer circuit wrappers (Triode CC, SE Output, PP, etc.) that reference that model.

## Recently completed
- [x] Restored analyser and pentode modeller to baseline behaviour after experimental kg1/curvature changes.
- [x] Added backend **SimpleManualPentode** model type, wired it into ModelFactory / Options, and updated it to use a web-style `epk` anode-current formula.
- [x] Implemented **Simple Manual Pentode** Modeller popup UI (sliders for `mu, kp, kg1, kg2, alpha, beta, gamma, a`), seeded from `Estimate::estimatePentode`, and wired plotting so that the saved instance is used when reselecting the model in the project tree.
- [x] Stabilised Gardiner/Reefman pentode fitting: reapply deferred bounds to all solve stages and null-initialise parameter array for safe logging (2025-11-13).
- [x] Designer (Single-Ended Output): extend SE Designer circuit beyond Pout/Vk/Ik/Rk/headroom/THD by adding **max symmetric clean swing (Vpp_sym)** and **max possible swing (Vpp_max)** metrics, and draw corresponding blue (symmetric) and brown (max) zones on the SE AC load line. Sym Swing checkbox now selects which swing mode drives the effective headroom and overlay colours.
- [x] Designer (Single-Ended Output): compute SE stage **input sensitivity** using the effective headroom swing and appropriate small-signal gain (respecting K-bypass mode), display the active sensitivity value in the Designer panel, and color the text to match the active swing mode (blue/brown for helpers, bright blue for manual override). Triode CC behaviour remains unchanged.
- [x] Designer/Modeller small-signal and harmonics (2025-11-22):
    - Modeller triode μ/gm/ra now use a local least-squares estimator over measurement data at the ~0.5·Ia_max operating point, with the same OP used for model μ/gm/ra and clear mes/mod vs Designer override colour cues on the Modeller small-signal LCDs.
    - SE Designer harmonic panel (HD2/3/4/THD) now uses the same time-domain DFT helper as the Harmonics tab, and SE **input sensitivity (Vpp)** is computed from effective headroom and correctly scaled gm, with K-bypass selecting bypassed vs unbypassed gain rather than collapsing to zero.
- [x] 2025-11-28: Documented task-tracking and commenting rules in `handoff.md` and `README.md`; clarified that `tasks.md` must be updated with each non-trivial change.
- [x] 2025-11-30: File → Export to Spice now defaults to a dedicated `models/spice` directory while still using a native Explorer-style save dialog.
- [x] 2025-11-30: File → Export Model (Export to Device) now defaults into the resolved models directory, uses `.json` by default, and offers a richer Explorer-style save filter (JSON/VWM/All).
- [x] 2025-11-30: Modeller Export-to-Devices button now prompts with a QFileDialog save menu, defaulting to the models directory with a suggested JSON device name.
- [x] 2025-11-30: File menu export actions mapped so **Export to Device…** uses the same helper as the Modeller Export-to-Devices button, and **Export Model to Spice…** exports a tube-only SPICE subcircuit for the selected Designer device.
- [x] 2025-11-30: Added SE Output Designer circuit SPICE export (File → Export SE Output to SPICE…), writing a self-contained netlist with the fitted tube subcircuit and a resistive-load SE stage schematic.
- [x] 2025-11-30: Designer pentode model overlays now use the embedded Measurement’s grid/screen families (via `Model::plotModel`) when available, so the red fitted curves align with the black measurement sweeps on SE Output and other Designer plots; fall back to `Device::anodePlot` when no measurement is present.
- [x] 2025-11-30: SE Output Designer X-axis now uses `max(device.vaMax, 2×VB)` on first plot, so the AC/DC load lines and Pa_max hyperbola have enough horizontal headroom for ~2× supply swing.
- [x] 2025-11-30: Extended Pa_max (plate-dissipation) hyperbola overlays to additional Designer circuits (Pentode Common Cathode, Single-Ended UL Output, Push-Pull Output, Push-Pull UL Output) and aligned output-stage X-axis behaviour with the SE Output reference (`axisVaMax = max(device.vaMax, 2×VB)` on first plot).

## Change log (highlights)
- 2025-11-14: Further experimental Reefman/pentode plotting changes caused regressions; all such changes were reverted via VCS and baseline behaviour restored, with Gardiner as reference.
- 2025-11-13: Gardiner/Reefman pentode solver bounds/logging fix; curves now align with measurement families; docs scheduled for refresh.
- 2025-11-13: Pentode plotting experiments (kg1 anchor/curvature) tried and reverted; SimpleManualPentode backend added as a new, manual model path; UI work pending.
- 2025-11-02: Save-to-Project dialog prompts every time; modelling grid-polarity guard
- 2025-10-21: Model plot loop fixed (correct grid stepping and bounds)

## Notes
- A = red (Triode A), B = green (Triode B)
- Measurements save into a Project; Modeller uses the selected measurement
- Every non-trivial code or documentation change must be recorded here by updating *Active tasks* or *Recently completed* with a short, dated note.

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada

## Future Projects – Pentode Class A1 Designer Parity

Reference: `refrence code/pentodeClassA1Designer-main`.

- **Pd limit curve overlay (status)**
  - 2025-11-30: SE Output, Pentode Common Cathode, Single-Ended UL Output, Push-Pull Output, and Push-Pull UL Output now draw a Pa_max hyperbola (`Ia = Pa_max * 1000 / Va`) over the Designer plots. Any new Designer circuits should follow the same pattern.

- **Main and alternate load lines**
  - Draw the main AC load line for the current tube, supply, bias, and load.
  - Add alternate load lines for `RL/2` and `2·RL` (and possibly other ratios).

- **Inductive vs resistive load modes**
  - Implement a mode toggle that changes the DC bias point and load-line geometry for transformer vs resistive load.

- **Bias point marker**
  - Show a clear marker at the DC operating point on the main load line.

- **Screen-current overlay**
  - Plot screen-current families with plate curves for pentode analysis.

- **Axis scaling controls**
  - Autoscale/fixed Y toggle.
  - Smarter automatic X-max tied to supply voltage.

- **Interactive swing/power measurement**
  - Click–drag on the plot to measure `ΔV`, `ΔI`, and approximate Class A power over that swing.

- **Tube preset library**
  - Add a tube preset data file (similar to `tubes.csv`) with Pmax, recommended Vplate, Vscreen, Ibias, and load.
  - Bind Designer controls to presets.

- **SPICE export-only workflow**
  - Generate SPICE netlists for Designer circuits (SE output, PP, Triode CC, etc.) without running ngspice internally, suitable for opening directly in LTspice/ngspice or other simulators.

- **Optional SPICE-backed curve mode**
  - Add an option to generate curves via ngspice from a netlist and `.inc` models, for comparison with internal models.

- **UI/UX polish inspired by reference**
  - Explore a compact “Pentode Class A1” view (toolbar+plot).
  - Dual numeric+slider controls for main parameters.
  - Keyboard shortcuts for fine adjustments.
  - Simple “Inductive” checkbox that changes both math and labels.
