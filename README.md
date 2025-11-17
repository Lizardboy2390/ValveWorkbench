# Valve Workbench by AudioSmith

Professional software for measuring, modeling, and designing vacuum tube (thermionic valve) circuits.

Brand: AudioSmith — Darrin Smith, Nelson BC, Canada

## Who this is for
- End users measuring real tubes with an Analyzer
- Builders and designers needing quick load lines and bias points
- Engineers fitting models to measured data for simulation and design

## What it does
- Analyser: Automates tube measurements with the Valve Wizard Valve Analyzer
- Modeller: Fits mathematical models (Ceres Solver) to your measurements
- Designer: Plots load lines, operating points, gain, and exports SPICE

## Quick start (Windows)
1) Requirements
   - Qt 6.5+ (MSVC) and CMake or qmake
   - Ceres Solver (prebuilt or vcpkg) and Eigen
   - Valve Wizard Valve Analyzer hardware (for measurements)
2) Build
   - qmake: open `ValveWorkbench.pro` in Qt Creator → Build
   - or CMake: configure to find Qt and Ceres → Build
3) Run
   - Launch ValveWorkbench
   - Connect hardware (optional for Analyser)

## Using the app
- Analyser tab
  - Select device type, set sweep ranges, click Run
  - Load/Save Template to reuse analyser settings
  - Save to Project to keep a measurement (dialog prompts for project name every time)
  - Note: Heater controls are not present; heaters are fixed in the new hardware
- Modeller tab
  - Select a measurement → Fit Triode/Pentode
  - Red curves (Triode A) and Green curves (Triode B) overlay when available
  - Positive grid voltages are auto-corrected negative for fitting
  - Export to Devices: after fitting, click "Export to Devices" to save the fitted model preset JSON into `models/` and refresh device dropdowns in Designer
- Designer tab
  - Choose circuit (e.g., Triode Common Cathode)
  - Adjust parameters and see load lines and operating point
  - Export SPICE netlist when ready
  - Device selection: pick a device from the dropdown; if "Show Fitted Model" is checked, red model curves will auto-plot even without measurements
  - Toggles (bottom row):
    - Show Measurement (measurement curves)
    - Show Fitted Model (model curves)
    - Show Designer Overlays (anode/cathode/AC/OP designer lines)
    - Show Screen Current (pentode only; hidden for triode circuits)
  - Available Designer circuits (subset wired as of 2025-11-16):
    - Triode Common Cathode (TriodeCC)
    - Pentode Common Cathode (PentodeCC)
    - AC Cathode Follower
    - DC Cathode Follower
    - Single-Ended Output (pentode)
    - Single-Ended UL Output (pentode, UL tap)
    - Push-Pull Output (pentode)
    - Push-Pull UL Output (pentode, UL tap)

## Features
- Double‑triode workflow with A/B overlays and color‑coded labels
- Robust fitting with multiple model families (Triode/Pentode)
- Pentode support:
  - **Gardiner Pentode** — stable Ceres-based pentode fit used as the reference model in the main branch.
  - **Reefman Pentode (Derk / DerkE)** — experimental models based on Derk Reefman’s work; long-term goal is to bring their behaviour closer to the standalone ExtractModel tool at `C:\Users\lizar\Documents\ExtractModel_3p0`. These are not currently recommended for everyday fitting.
  - **Simple Manual Pentode** — a manual pentode modeller using the same web-style `epk` anode-current formula as the web pentode modeller. The backend is implemented; UI sliders/numeric controls on the Modeller tab will allow you to set parameters (`mu, kp, kg1, kg2, alpha, beta, gamma, a`, etc.) by hand and see red curves update live over your measurements, without running Ceres.
  - **Triode-Connected Pentode analyser mode** — analyser device type that drives a pentode’s anode and screen together (S3/S7 track) to generate triode-like anode characteristics for that tube, used as a high-quality triode seed for pentode fitting.
- Real‑time plotting and responsive UI
- SPICE netlist export

## Pentode modelling notes

- In the **mainline** build, Gardiner is the stable, auto-fit pentode model. If pentode fits look wrong after an experiment, revert to a commit where Gardiner overlays are correct and treat that as the baseline.
- **Simple Manual Pentode** is intended as a manual slider-based modelling path. When the sliders are wired, you will be able to:
  - Select Simple Manual Pentode in Options.
  - Adjust parameters on the Modeller tab and see the model curves move in real time.
  - Save a manually tuned parameter set via "Export to Devices" for use in Designer.
- **Reefman Pentode** is experimental. Future work to align it with ExtractModel_3p0 should be done on a separate branch and validated against that tool’s outputs before it is used for regular fitting.
- Pentode fits now use **centralised parameter bounds** in `Model::setEstimate`, with different envelopes for **Gardiner vs Reefman** models. Gardiner keeps a broad but stable range; Reefman/DerkE uses tighter UTmax-style bounds to keep the DEPIa-style model in a realistic corridor.
- For triode-based seeding of pentode models, the analyser provides a **Triode-Connected Pentode** device type. Measurements taken in this mode appear in the project tree as `Triode (Triode-Connected Pentode) Anode Characteristics` and are used as the triode source for `Estimate::estimatePentode`.

### Recommended pentode measurement & fitting workflow

1. **Measure triode-connected pentode curves (seed)**
   - Analyser tab:
     - Device Type: `Triode-Connected Pentode`.
     - Test: `Anode Characteristics`.
     - Set `Va`, `Vg1` ranges and limits as usual.
   - Run the test and click **Save to Project**.
   - In the project tree, this measurement will appear as `Triode (Triode-Connected Pentode) Anode Characteristics`.

2. **Fit a triode model from the triode-connected measurement**
   - Modeller tab:
     - Select the `Triode (Triode-Connected Pentode)` measurement.
     - Click **Fit Triode**.
   - This creates a triode model node in the project, used as the UTmax-style seed for pentode fitting.

3. **Measure normal pentode curves**
   - Analyser tab:
     - Device Type: `Pentode`.
     - Test: `Anode Characteristics` (and/or `Transfer Characteristics`).
   - Run the test(s) and **Save to Project**.

4. **Fit the pentode model (Gardiner or Reefman)**
   - Modeller tab:
     - Select the pentode measurement.
     - Choose the desired pentode model type (typically **Gardiner Pentode** for production fits).
     - Click **Fit Pentode**.
   - The fitter uses the triode model from step 2 as the seed and applies the model-specific bounds described above.

5. **Persist fitted parameters back into the model JSON**
   - In the project tree, click the fitted pentode model node (e.g. `Gardiner Pentode`).
   - On the Modeller tab, click **Export to Devices**.
   - Choose the appropriate device file in `models/` (for example `EL34.json`, `6L6.json`, `6V6-S.json`).
   - Confirm overwrite if prompted.
   - On the next application start, the updated JSON parameters become the default for that device in Modeller/Designer and are used when computing load lines.

## Troubleshooting
- No “Save to Project” prompt: The app now prompts every time before saving
- Modeller fails to converge: Ensure grid voltages are within range; guard auto‑flips positive Vg to negative
- Empty plots in Designer: Select a device model first
- No model curves in Designer: Ensure a device is selected and "Show Fitted Model" is checked
- Designer overlays extend beyond model curves: Axes are clamped to device `vaMax`/`iaMax`; ensure your device preset has correct limits
- Pentode fits look flatter than datasheets: this is expected with the current Gardiner auto-fit. For precise, datasheet-like curves, the planned Simple Manual Pentode modeller (web-style `epk` backend, manual sliders) will allow you to match curves by hand. Reefman models are experimental and may not converge on all data sets yet.

## Change log (highlights)
- 2025‑11‑16: Designer circuits: added Pentode Common Cathode, AC/DC Cathode Follower, Single-Ended and Single-Ended UL outputs, Push-Pull and Push-Pull UL outputs; Designer plot now fully clears and resets overlays when switching circuits; Export to Devices now writes both `analyserDefaults` and `model` to preset JSONs so Designer and Analyser share a single device profile.
- 2025‑11‑15: Added Triode-Connected Pentode analyser device type; triode-connected pentode measurements stored as triode tests with clear hints; centralised Gardiner vs Reefman pentode bounds aligned with UTmax-style seeding.
- 2025‑11‑05: Modeller "Export to Devices"; Designer overlays checkbox; auto model plotting on device select; axes clamped to device limits; screen current toggle pentode‑only
- 2025‑11‑02: Analyser: removed heater button/LCDs (heaters fixed in hardware)
- 2025‑11‑02: Analyser: Load/Save Template for analyser settings
- 2025‑11‑02: Always prompt for Project name before saving measurements
- 2025‑11‑02: Modeller guard flips positive grid voltages to negative during fitting
- 2025‑10‑21: Fixed infinite loop in model plotting (grid stepping correction)

## Credits & Contact
- AudioSmith — Darrin Smith, Nelson BC, Canada
- For support: please include OS, Qt version, and steps to reproduce

## License
This project contains third‑party components. See `included-licences/` for details.
