# Valve Workbench — Tasks & Change Log (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Active tasks (end‑user focused)
- [ ] Show Fitted Model: route checkbox to Model::plotModel for pentode (use measured Vg2, kg1 anchor, curvature)
- [ ] Curvature refinement: estimate beta/gamma from low‑Va region (plotting‑only) for better bend
- [ ] Scale robustness: remove temporary iaScale once units are consistently mA end‑to‑end
- [ ] Designer: ensure device selection auto‑replots and axes clamp to device limits
- [ ] README: add “Pentode plotting troubleshooting” (units, families, checkbox)

## Recently completed
- [x] Pentode plotting uses 3‑arg current with measured Vg2 (no triode fallback)
- [x] Vg1 family units fixed (use measurement −20…−60 V; no mV→V conversion)
- [x] Muted Cohen‑Helie debug logs when called from pentode
- [x] First‑pass kg1 calibration from −20 V family mid‑point
- [x] Plot guards: clamp Ia to axis, skip extreme/zero families, avoid verticals
- [x] Initial plotting‑only curvature parameters (a/beta/gamma)

## Change log (highlights)
- 2025‑11‑13: Pentode plotting path corrected; kg1 anchor + curvature; unit fixes; checkbox wiring pending
- 2025‑11‑02: Save‑to‑Project dialog prompts every time; modelling grid‑polarity guard
- 2025‑10‑21: Model plot loop fixed (correct grid stepping and bounds)

## Notes
- A = red (Triode A), B = green (Triode B)
- Measurements save into a Project; Modeller uses the selected measurement

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
