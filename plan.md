# Valve Workbench — Project Plan (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Purpose
Deliver a stable, end‑user application for measuring, modeling, and designing vacuum‑tube circuits with clear workflows for Single and Double Triode devices.

- Modeller: Pentode fitting now logs cleanly and overlays align with measurements after parameter-array initialization fix
- Modeller threading: Bounds now applied to anode, screen, and remodel passes; no "parameter block not found" crashes observed
- Analyser: Automated sweeps remain stable; Save to Project prompts every time
- Designer: Triode Common Cathode enhancements completed (2025‑11‑06): axes clamp, overlays cleared, gain/swing UI refreshed

- Reliability: Verify full measurement → save → pentode fit → overlay workflow with new logging/bounds
- Pentode UX: surface manual/automatic fitting options (Simple Manual Pentode sliders TBD)
- Documentation: End‑user README, plan, tasks, handoff kept current with pentode progress

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
  - Triode CC Designer overlays and UI complete (Max/Max Sym Swing, Input Sensitivity)
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
- Gardiner/Reefman path: parameter bounds now applied across solve stages, logging cleaned up, overlays match measurement families
- **Simple Manual Pentode** backend exists (web-style `epk` formula, no auto-fit yet)
- Next: expose manual parameters in UI (sliders / numeric inputs) and replot live over measured data
- Later: once manual path mirrors web tool for representative tubes (6L6, EL34), prototype minimal auto-fit over a subset of parameters

## Change log (highlights)
- 2025‑11‑06: Designer (Triode CC) overlays complete (Max Swing brown / Max Sym Swing blue); Input sensitivity into Designer; K bypass + single Gain; gm/ra formatting; cathode line clipping; Pa‑max entry fix; docs updated
- 2025‑11‑05: Export to Devices (Modeller); Designer overlays toggle; auto model plotting; axes clamped; docs updated
- 2025‑11‑02: Always prompt for project name before save; modelling guard for positive Vg
- 2025‑10‑21: Fixed infinite loop in model plotting (grid step direction)

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
