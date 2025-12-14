# Valve Workbench — Tasks & Change Log (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Active tasks (end-user focused)
- [ ] README / docs: clearly describe that Gardiner is the stable reference pentode fit in `main`, that **Simple Manual Pentode** is the manual slider-based path, and that Reefman/uTracer/ExtractModel work belongs on an experimental branch.
- [ ] Integrated SPICE export via Devices:
  - Extend **Export model to Device** so each device JSON optionally includes a SPICE representation of the fitted tube model (triode and pentode) suitable for `.inc`/`.subckt` use.
  - Implement **File → Export to Spice...** so it uses the active Device's SPICE block to export (a) tube model only (`.inc`/`.lib`), and (b) optional Designer circuit wrappers (Triode CC, SE Output, PP, etc.) that reference that model.
- [ ] Datasheet reference stats + tube health metric:
  - Add a Designer-side "Datasheet / Reference" group box to hold one or more datasheet operating points (Va, Vg, Ia, gm, μ, rp) for each tube/section, with values stored in templates.
  - Extend template JSON (Load/Save Template and Export-to-Devices) to persist a nested `datasheet.refPoints[]` block carrying these reference stats alongside analyserDefaults and measurement.
  - Implement a helper that, given a Measurement and a reference point, computes measured Ia/gm/μ/rp at (or near) the datasheet operating point and derives a simple tube "health %" from the ratios.
  - Surface the health metric and reference vs measured values in the UI (Designer/Modeller panel and/or a small status readout) so users can quickly gauge tube strength and life.
  - Use the eTracer PC software manual in `refrence code/ilovepdf_pages-to-jpg (1)` (quick scan, corners tests, Imax/Pmax usage) as the primary reference for feature design and defaults.

## Recently completed
- [x] 2025-12-13: Pentode Health transfer sweeps: fixed one-sample sweeps caused by immediately hitting the 50 mA hard clamp by biasing the Health transfer `Vg1` window to start more negative for pentodes (so the sweep captures multiple points for gm estimation).
- [x] 2025-12-13: Pentode Health (6L6-GC): re-enabled as **Quick-mode only** by adding an automatic operating-point finder sweep that selects a safe Vg1 for a target current (default 15 mA for 6L6 family) at conservative Va/Vg2, then runs the standard Health transfer sweeps around that found OP.
- [x] 2025-12-14: Pentode Health UI: Triode A Health box title now switches to **Pentode Health** when a pentode template/device type is selected.
- [x] 2025-12-14: Pentode Screen Health UI: repurposed Triode B Health box as **Screen Health** (Ig2 / Pg2), hid the 4-corner column for pentodes, and fixed the Screen Health Ref column to use `datasheet.healthReference.center.ig2Ref_mA` (and derived Pg2) instead of mirroring Triode A ref values.
- [x] 2025-12-14: Pentode Health % columns: allow pentode datasheet `gm` to be omitted so Ia-based % still computes; also read optional pentode `rp`/`mu` from `datasheet.refPoints[0]` so rp%/mu% can populate when present.
- [x] 2025-12-14: Health UI consistency: fixed Full Health tooltip/status message to report the same displayed Full score (reference-tube score when present) as the `F` label.
- [x] 2025-12-14: Template persistence: fixed Save Template per-test analyser snapshot serialization to sanitise non-finite/denormal ranges and to force fixed Vg2 ranges for pentode anode/transfer snapshots (prevents corrupted analyserDefaults on reload).
- [x] 2025-12-13: Pentode modelling: fixed secondary-emission parameter seeding by passing `preferencesDialog.useSecondaryEmission()` into `Estimate::estimatePentode()` (previously hard-coded false), and updated the `Fix secondary emission` preference semantics so it freezes only SE geometry (`omega/lambda/nu`) while still allowing `S/Ap` to fit (prevents SE from effectively disappearing when the box is checked).
- [x] 2025-12-11: Compare dialog updated so that pentode test conditions and pentode metrics are only shown when at least one selected model is a pentode, and triode/pentode μ/gm/ra/Ia values now use the shared Model::computeSmallSignal helper at the user-specified test conditions for consistent small-signal behaviour.
- [x] 2025-12-11: Compare dialog triode/pentode metric tables extended with a fourth Δ column showing comparison-minus-reference differences for μ/gm/ra/Ia, and triode test conditions now auto-fill from the active datasheet.refPoints[0] operating point (Va, Vg) when available so Compare uses the same bias as Health and datasheet comparisons by default.
- [x] 2025-12-11: Analyser startup device name now shows an explicit "(no template loaded)" placeholder instead of defaulting to the first loaded Device name (e.g. 6N2P-EV) so users are not misled into thinking a specific template is active before loading one.
- [x] 2025-12-11: Analyser "Show Triode B" checkbox now correctly gates the Triode B measurement overlay for Double Triode devices (using the same logic as Modeller), so Triode B curves only appear when both Show Measurement and Show Triode B are enabled, and hide when either is turned off.
- [x] 2025-12-11: Export-to-Devices for triode presets now writes analyserDefaults.tests["anode"].grid from the analyser UI's positive-magnitude grid range (0..+V, positive step) instead of the negative measurement sweep, so templates keep intuitive 0→+V grid defaults while the embedded measurement block still preserves the true negative Vg range.
- [x] 2025-12-10: Designer **Triode Common Cathode** headroom/THD path upgraded to a sine-driven time-domain helper that drives the triode model at the self-bias grid voltage, solves Va(t) along the Ra‖Rl AC load line with Rk feedback reflected in the small-signal gain, and derives HD2/HD3/HD4/HD5/THD from a Hann-windowed Ia(t) waveform at the requested headroom amplitude.
- [x] 2025-12-10: Designer **Triode Common Cathode** Headroom Waveshape viewer wired to render a DC-removed, peak-normalised anode-voltage waveform Va(t) over one cycle at the effective headroom (manual or helper-derived), with increased time-domain sample count (1024 points), an enlarged QGraphicsView, and an auto-centered scene rect with vertical padding so the waveform is not clipped at the edges, updating whenever Triode CC recomputes headroom/THD, Max Sym Swing, or K-bypass.
- [x] 2025-12-10: Designer **Triode Common Cathode** sine-driven headroom/THD helper updated so the cathode resistor behaves dynamically at a nominal 400 Hz test tone: when K-bypass is **off** the instantaneous cathode voltage follows Ia(t)·Rk around the DC bias (local feedback reduces gain and distortion), and when K-bypass is **on** the cathode is treated as AC-ground so the higher-gain bypassed behaviour is reflected in Va(t), HD2/HD3/HD4/HD5/THD, and the Headroom Waveshape viewer.
- [x] 2025-12-09: Quick/Full Health gm estimator updated in `computeIaGmAt` to use a Vg-centred voltage window and bin-averaged local regression over all usable samples near each HealthPoint (reducing stair-step/see-saw artefacts on dense triode sweeps), Triode B Full Health corner scoring adjusted so gm-fitting failures no longer discard valid positive Ia (B corners remain Ia-only), and Modeller's measurement-based gm helper `gmFromTransferAtOP` updated to use the same Vg-window/binning regression for transfer data so Modeller gm/μ LCDs in measured mode are less affected by DAC stair-stepping.
- [x] 2025-12-04: Designer two-stage **Triode CC + DC cathode follower** circuit (`TriodeCcDccfTwoStage`, TEST_CALCULATOR) extended with a Stage 2 headroom (Vpk) parameter and a full VTADIY-style time-domain THD pipeline at the follower anode, exposing a Stage 2 THD-at-headroom (%) metric in the Designer panel while keeping Stage 1 gain computed but hidden for this circuit.
- [x] 2025-12-03: Quick/Full Health triode analyser orchestration implemented in `ValveWorkbench`: uses `datasheet.refPoints[0]` as the reference Va/Vg/Ia/gm point, configures short transfer sweeps around one (Quick) or five (Full) operating points, derives Ia and local gm from the measured sweeps, and reports percentage "health" scores in the status bar and on the Quick/Full Health buttons; behaviour remains experimental until validated on hardware.
- [x] 2025-12-03: Fixed Quick/Full Health crash after test completion by de-reentrifying health run chaining; `ValveWorkbench::testFinished()` now queues the next health sweep via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` instead of calling `on_runButton_clicked()` directly inside the analyser callback, and `Analyser::nextSample()` guards its progress calculation so there is no divide-by-zero if sweep/test state is reset between runs.
- [x] 2025-11-30: Designer device selection now auto-replots using `selectStdDevice` and `updateCircuitParameter` so that output-stage axes clamp to device limits and supply/load headroom: X uses `max(currentXStop, max(device.vaMax, 2×VB))` for SE/SE-UL/PP/UL-PP, Y uses `device.iaMax` or `max(device.iaMax, 4000·VB/RAA)` depending on circuit and Autoscale Y mode.
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
- [x] 2025-11-30: Extended Pa_max (plate-dissipation) hyperbola overlays to additional Designer circuits (Pentode Common Cathode, Single-Ended UL Output, Push-Pull Output, and Push-Pull UL Output) and aligned output-stage X-axis behaviour with the SE Output reference (`axisVaMax = max(device.vaMax, 2×VB)` on first plot).
- [x] 2025-11-30: Designer output-stage axes and model overlays updated for SE, SE-UL, PP, and UL-PP:
  - X-axis for all four circuits now auto-expands to at least `2×VB` on device select and VB edits, without shrinking on lower VB.
  - Y-axis for PP and UL-PP now auto-expands to cover the theoretical Class-B peak current (`Ia_classB ≈ 4000·VB/RAA`) while never shrinking below the device/plot range.
  - Fitted-model anode grids (red) for these circuits are plotted from `-vg1Max` up to `0 V` at ~2 V steps for pentodes, extend to the visible X-axis edge, and carry Modeller-style Vg labels placed ~70% along the visible line with a small gap cut out under each label.
  - Push-Pull Output plots include an orange max-power marker at the AC load line / `Vg1=0` intersection and an axes-aware Pa_max hyperbola that remains visible under extended Designer axes.

- [x] 2025-11-30: Designer Autoscale Y checkbox implemented for output-stage circuits (SE, SE-UL, PP, UL-PP). With Autoscale Y **enabled**, device select and VB/RAA edits recompute axes from the active device and circuit: X uses `max(currentXStop, max(device.vaMax, 2×VB))`, Y uses `device.iaMax` for SE/SE-UL and `max(device.iaMax, 4000·VB/RAA)` for PP/UL-PP. With Autoscale Y **disabled**, the current Y range is treated as locked while X can still auto-extend to at least `2×VB`.

- [x] 2025-12-13: Designer time-domain headroom helpers aligned across **Triode Common Cathode**, **Single-Ended Output**, and **Push-Pull Output** so that effective headroom (manual or helper-derived) maps to a single grid-drive Vpp via small-signal gain (no iterative Vpp rescaling), letting the model/load-line physics shape clipping. Triode CC and SE Output continue to feed the shared Headroom Waveshape viewer via their Va(t) buffers, and Push-Pull Output now exposes an approximate Va(t) waveshape (derived from its 5-point VTADIY-style current helper and DC load line) to the same viewer and participates in the Harmonics tab Headroom Scan as a third circuit option.

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
  - Autoscale/fixed Y toggle. **Status:** Implemented as a Designer “Autoscale Y” checkbox. When checked, SE, SE-UL, PP, and UL-PP recompute their Y-axis from the active device and circuit on device select and VB/RAA edits (SE/SE-UL: `device.iaMax`; PP/UL-PP: `max(device.iaMax, 4000·VB/RAA)`). When unchecked, the current Y range is preserved while X can still auto-extend.
  - Smarter automatic X-max tied to supply voltage. **Status:** Output-stage circuits (SE, SE-UL, PP, UL-PP) now auto-extend X to `max(device.vaMax, 2×VB)` on device select and VB changes, without shrinking when VB is reduced.

- **Interactive swing/power measurement**
  - Click–drag on the plot to measure `ΔV`, `ΔI`, and approximate Class A power over that swing.

- **Tube preset library**
  - Add a tube preset data file (similar to `tubes.csv`) with Pmax, recommended Vplate, Vscreen, Ibias, and load.
  - Bind Designer controls to presets.

- **SPICE export-only workflow**
  - Generate SPICE netlists for Designer circuits (SE output, PP, Triode CC, etc.) without running ngspice internally, suitable for opening directly in LTspice/ngspice or other simulators.

- **UI/UX polish inspired by reference**
  - Explore a compact “Pentode Class A1” view (toolbar+plot).
  - Dual numeric+slider controls for main parameters.
  - Keyboard shortcuts for fine adjustments.
  - Simple “Inductive” checkbox that changes both math and labels.
