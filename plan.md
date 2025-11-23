# Valve Workbench — Project Plan (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Purpose
Deliver a stable, end‑user application for measuring, modeling, and designing vacuum‑tube circuits with clear workflows for Single and Double Triode devices.

- Modeller: Pentode fitting now logs cleanly and overlays align with measurements after parameter-array initialization fix
- Modeller threading: Bounds now applied to anode, screen, and remodel passes; no "parameter block not found" crashes observed
- Analyser: Automated sweeps remain stable; Save to Project prompts every time
- Designer: Triode Common Cathode enhancements completed (2025‑11‑06): axes clamp, overlays cleared, gain/swing UI refreshed

- Reliability: Verify full measurement → save → pentode fit → overlay workflow with the restored baseline (no experimental Reefman/bounds changes)
- Pentode UX: surface manual/automatic fitting options (Simple Manual Pentode sliders TBD, manual-first)
- Documentation: End‑user README, plan, tasks, handoff kept current with pentode progress and branch policy for experimental pentode work (Reefman/ExtractModel alignment on a dedicated branch)

## Milestones
- M1: Workflow baseline (complete)
  - Save to Project prompts every time
  - Modeller guard for grid polarity
  - A/B overlay colour coding
- M2: Polishing & stability (in progress)
  - Robust plotting in Designer and Modeller (no empty or runaway plots)
  - Helpful error messages for invalid ranges/ports
  - Export to Devices button on Modeller; refresh device list in Designer
  - Designer overlays cleared before replot; triode hides Screen Current
  - Auto model plotting on device select (Show Fitted Model)
  - Axes clamped to device `vaMax`/`iaMax`
  - Triode CC Designer overlays and UI complete (Max/Max Sym Swing, Input Sensitivity, shared OP with Modeller μ/gm/ra and mes/mod colour cues)
  - Triode CC and SE Designer small-signal/harmonic work complete
- M3: Documentation & Release prep (in progress)
  - Updated README (end user), plan, tasks
  - Curated change log and screenshots

## Risks & mitigations
- Hardware variance → Provide safe defaults and sanity checks
- Fit convergence issues → Initial parameter hints and Vg guard (done)
- UI confusion → Consistent colours/labels (A=red, B=green) and prompts

- Immediate: Add Simple Manual Pentode UI controls for manual curve alignment, preserving stable Gardiner/Reefman baseline
- Next: Evaluate Vg label gating (labels only if family has >0 mA segment in bounds); keep overlay clearing/axes clamp pattern for remaining circuits
- Later: Add explicit "Calculate" button in Designer; refresh README screenshots once pentode UI finalizes

### Pentode Modelling (in progress)
- Gardiner path: parameter bounds applied across solve stages, logging cleaned up, and overlays match measurement families in the restored baseline. Gardiner remains the stable reference Ceres-based pentode fit in `main`.
- Reefman path: still non-convergent/unstable on current measurements; treat as experimental only and keep any further work on a dedicated branch.
- **Simple Manual Pentode**: backend `epk` formula implemented **and** Modeller popup UI wired (sliders for `mu, kp, kg1, kg2, alpha, beta, gamma, a`). Simple Manual Pentode can now be selected as the pentode fit mode, seeded from the same `Estimate::estimatePentode` used for Gardiner, and adjusted live via sliders.
- Current limitation: Simple Manual Pentode `anodeCurrent` scaling is not yet calibrated to the global mA convention; first-pass curves from the slider model do not reliably match Gardiner/measurement magnitude. Use Gardiner for production fits until a dedicated calibration pass is done.
- Experimental (branch-only): on a dedicated branch (e.g. `feature/reefman-extractmodel`), work toward aligning Reefman pentode behaviour with the standalone ExtractModel_3p0 tool at `C:\Users\lizar\Documents\ExtractModel_3p0` by comparing Ia/Ig2 at selected operating points.
- Later: once the Simple Manual Pentode path mirrors the web tool and Gardiner in both shape and scale for representative tubes (6L6, EL34), consider a minimal auto-fit layer over a subset of parameters.
- New: **Triode-Connected Pentode** analyser mode added as a dedicated device type. This drives the pentode’s anode and screen together to generate triode-like anode characteristics per tube, which are then used as high-quality triode seeds for `Estimate::estimatePentode`.
- New: Centralised pentode parameter bounds in `Model::setEstimate` now distinguish between **Gardiner** (wider, stable envelope) and **Reefman/DerkE** (tighter UTmax-style corridor) to improve fit stability without constraining the main Gardiner path.
- New (SE Designer): Single-Ended Output harmonic panel and Harmonics tab now share a **sine-driven, grid-excited time-domain engine** for HD2/3/4/5 and THD. A pure sine is applied at the grid, the model + AC load line are solved per sample via `findVaFromVg` to obtain Va(t)/Ia(t), and a Hann-windowed DFT extracts harmonic magnitudes. SE input sensitivity is computed from effective headroom and a correctly scaled gm·Ra gain (with K-bypass selecting bypassed vs unbypassed gain).
- New: **Tube-style device presets**: Export to Devices now writes a single JSON that contains both the fitted `model` and the originating `measurement` sweeps (when the export comes from a measurement-based fit). Designer SE output prefers this embedded measurement to derive idle Vk/Ik/Ig2 numerically from data, with a safe fallback to the tube model for legacy or hand-authored presets.

#### Option 3 (future): Manual Gardiner-style pentode
- Longer term, consider a separate "Manual Gardiner Pentode" model that:
  - Shares Gardiner's unified Ia expression and full parameter set (mu, kg1, kg2/kg2a, kp, kvb, kvb1, vct, a, alpha, beta, gamma, os, etc.).
  - Is driven purely by Modeller sliders/fields (no Ceres solve), optionally **seeded** from `Estimate::estimatePentode` so the initial curves match the automatic Gardiner fit.
  - Acts as a manual debug/tuning view of the working Gardiner model, avoiding the unit/scale mismatch inherent in the current Simple Manual Pentode epk formulation.

## Change log (highlights)
- 2025‑11‑06: Designer (Triode CC) overlays complete (Max Swing brown / Max Sym Swing blue); Input sensitivity into Designer; K bypass + single Gain; gm/ra formatting; cathode line clipping; Pa‑max entry fix; docs updated
- 2025‑11‑05: Export to Devices (Modeller); Designer overlays toggle; auto model plotting; axes clamped; docs updated
- 2025‑11‑02: Always prompt for project name before save; modelling guard for positive Vg
- 2025‑10‑21: Fixed infinite loop in model plotting (grid step direction)

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
