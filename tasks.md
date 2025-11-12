# Valve Workbench — Tasks & Change Log (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Active tasks (end‑user focused)
- [ ] Improve Designer plot robustness (no empty graphs, clear messages)
- [ ] Expand troubleshooting section in README (USB/ports, convergence)
- [ ] Add sample projects and screenshots
- [ ] Verify double‑triode property tables and legends
- [ ] Optional: Auto‑save calibration measured values on change (in addition to OK)
- [ ] Optional: Expose a UI toggle to disable auto‑open on Run

## Recently completed
- [x] Preferences: grid calibration (−5/−60 V) immediate apply when idle, pipelined during test
- [x] Calibration persistence via QSettings; normalization/reset behaviors for measured values
- [x] Ensure serial port auto‑opens on Run; warn and abort if unavailable
- [x] Always prompt for project name before saving measurements
- [x] Modeller guard: flip positive grid voltages to negative during fitting
- [x] Fix model plotting infinite loop (grid stepping)

## Change log (highlights)
- 2025‑11‑12: Preferences calibration finalized; persistence implemented; Run auto‑opens port; analyser/valveworkbench wiring
- 2025‑11‑02: Save‑to‑Project dialog prompts every time; modelling grid‑polarity guard
- 2025‑10‑21: Model plot loop fixed (correct grid stepping and bounds)

## Notes
- A = red (Triode A), B = green (Triode B)
- Measurements save into a Project; Modeller uses the selected measurement

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
