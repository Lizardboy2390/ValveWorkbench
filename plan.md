# Valve Workbench — Project Plan (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Purpose
Deliver a stable, end‑user application for measuring, modeling, and designing vacuum‑tube circuits with clear workflows for Single and Double Triode devices.

## Current state (high level)
- Analyser: Automated sweeps working; Save to Project prompts every time
- Modeller: Fitting stable; guard flips positive grid voltages to negative
- Designer: Triode Common Cathode functional; device selection and plotting work

## Goals (near‑term)
- Reliability: Validate measurement → save → model → compare workflow
- Double‑Triode UX: Clear A/B overlays, colors, and labels throughout
- Documentation: End‑user README, concise tasks, visible change log

## Milestones
- M1: Workflow baseline (complete)
  - Save to Project prompts every time
  - Modeller guard for grid polarity
  - A/B overlay colour coding
- M2: Polishing & stability (in progress)
  - Robust plotting in Designer and Modeller (no empty or runaway plots)
  - Helpful error messages for invalid ranges/ports
- M3: Documentation & Release prep (in progress)
  - Updated README (end user), plan, tasks
  - Curated change log and screenshots

## Risks & mitigations
- Hardware variance → Provide safe defaults and sanity checks
- Fit convergence issues → Initial parameter hints and Vg guard (done)
- UI confusion → Consistent colours/labels (A=red, B=green) and prompts

## Roadmap (rolling)
- Week 1: Stabilize double‑triode visuals and property tables
- Week 2: Validate Designer calculations; add SPICE examples
- Week 3: Expand troubleshooting; refine error dialogs

## Change log (highlights)
- 2025‑11‑02: Always prompt for project name before save; modelling guard for positive Vg
- 2025‑10‑21: Fixed infinite loop in model plotting (grid step direction)

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada  Darrin@AudioSmith.ca

