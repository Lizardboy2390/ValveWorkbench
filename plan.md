# Valve Workbench — Project Plan (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Purpose
Deliver a stable, end‑user application for measuring, modeling, and designing vacuum‑tube circuits with clear workflows for Single and Double Triode devices.

## Current state (high level)
- Analyser: Automated sweeps working; Save to Project prompts every time
- Modeller: Fitting stable; guard flips positive grid voltages to negative
- Designer: Triode Common Cathode enhancements completed (2025‑11‑06):
  - Max swing (upper row, brown) and Max Sym Swing (lower row, blue) overlays
  - Input sensitivity (Vpp) moved to Designer values; depends on Max Sym Swing; updates with K bypass
  - Single Gain box driven by K bypass; gm/ra formatting (gm 2dp, ra integer)
  - Cathode line now purple and clipped from left/right margins; Pa‑max line starts inside y‑range
  - Overlays clear on replot; axes clamped to device limits

## Goals (near‑term)
- Reliability: Validate measurement → save → model → compare workflow
- Double‑Triode UX: Clear A/B overlays, colors, and labels throughout
- Documentation: End‑user README, concise tasks, visible change log (updated 2025‑11‑06)

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

## Roadmap (rolling)
- Tomorrow: Implement minimal, safe Vg label gating (labels only if family has >0 mA segment in-bounds); place labels at edge intersection; compile and test
- Next: Apply overlay clearing/axes clamp pattern to remaining circuits
- Then: Add explicit "Calculate" button in Designer; screenshot update for README

## Change log (highlights)
- 2025‑11‑06: Designer (Triode CC) overlays complete (Max Swing brown / Max Sym Swing blue); Input sensitivity into Designer; K bypass + single Gain; gm/ra formatting; cathode line clipping; Pa‑max entry fix; docs updated
- 2025‑11‑05: Export to Devices (Modeller); Designer overlays toggle; auto model plotting; axes clamped; docs updated
- 2025‑11‑02: Always prompt for project name before save; modelling guard for positive Vg
- 2025‑10‑21: Fixed infinite loop in model plotting (grid step direction)

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
