# Valve Workbench — Tasks & Change Log (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Active tasks (end‑user focused)
- [ ] Modeller UI: add sliders / numeric fields for Simple Manual Pentode parameters (`mu, kp, kg1, kg2, alpha, beta, gamma, a`) and replot curves live over measured pentode data.
- [ ] Optional: once Simple Manual Pentode matches the web tool visually for key tubes, add a small auto-fit layer on top (fit a subset of parameters only).
- [ ] Designer: ensure device selection auto‑replots and axes clamp to device limits.
- [ ] README: briefly describe Simple Manual Pentode and note that legacy Gardiner/Reefman pentode fits are unchanged.

## Recently completed
- [x] Restored analyser and pentode modeller to baseline behaviour after experimental kg1/curvature changes.
- [x] Added backend **SimpleManualPentode** model type, wired it into ModelFactory / Options, and updated it to use a web-style `epk` anode-current formula (no sliders or auto-fitting yet).

## Change log (highlights)
- 2025‑11‑13: Pentode plotting experiments (kg1 anchor/curvature) tried and reverted; SimpleManualPentode backend added as a new, manual model path; UI work pending.
- 2025‑11‑02: Save‑to‑Project dialog prompts every time; modelling grid‑polarity guard
- 2025‑10‑21: Model plot loop fixed (correct grid stepping and bounds)

## Notes
- A = red (Triode A), B = green (Triode B)
- Measurements save into a Project; Modeller uses the selected measurement

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
