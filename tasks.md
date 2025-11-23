# Valve Workbench — Tasks & Change Log (AudioSmith)

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Active tasks (end-user focused)
- [ ] Validate measurement → save → pentode fit → overlay workflow using the restored baseline (no experimental Reefman/bounds changes); use Gardiner as the visual reference for pentode auto-fit.
- [ ] Calibrate **Simple Manual Pentode** current scaling so that `anodeCurrent` returns mA in the same numeric range as measurement and Gardiner/Reefman for representative tubes (e.g. 6L6). Remove ad-hoc empirical scale factors once a stable mapping is found.
- [ ] Optional: once Simple Manual Pentode matches the web tool and Gardiner visually for key tubes (e.g. 6L6, EL34), add a small auto-fit layer on top that adjusts only a subset of parameters while keeping the manual feel.
- [ ] Designer: ensure device selection auto-replots and axes clamp to device limits.
- [ ] README / docs: clearly describe that Gardiner is the stable reference pentode fit in `main`, that **Simple Manual Pentode** is the manual slider-based path, and that Reefman/uTracer/ExtractModel work belongs on an experimental branch.
- [ ] Experimental branch: on a separate branch (e.g. `feature/reefman-extractmodel`), work toward aligning `ReefmanPentode` behaviour with ExtractModel_3p0 located at `C:\Users\lizar\Documents\ExtractModel_3p0`, using common parameter sets and sample Ia/Ig2 points as reference.

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

## Change log (highlights)
- 2025-11-14: Further experimental Reefman/pentode plotting changes caused regressions; all such changes were reverted via VCS and baseline behaviour restored, with Gardiner as reference.
- 2025-11-13: Gardiner/Reefman pentode solver bounds/logging fix; curves now align with measurement families; docs scheduled for refresh.
- 2025-11-13: Pentode plotting experiments (kg1 anchor/curvature) tried and reverted; SimpleManualPentode backend added as a new, manual model path; UI work pending.
- 2025-11-02: Save-to-Project dialog prompts every time; modelling grid-polarity guard
- 2025-10-21: Model plot loop fixed (correct grid stepping and bounds)

## Notes
- A = red (Triode A), B = green (Triode B)
- Measurements save into a Project; Modeller uses the selected measurement

## Contact
AudioSmith — Darrin Smith, Nelson BC, Canada
