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
  - Triode μ estimation for modelling:
    - For triode and triode-connected pentode measurements, the internal Cohen–Helie triode seed uses a **common operating point** slice at `Ia ≈ 0.5 × Ia_max` (with a 1 mA minimum), instead of the older 5% slice.
    - Along that slice the code interpolates, for each grid sweep, the anode voltage `Va` where the current equals that slice (`iMu`) and collects `(Vg1_nominal, Va_at_iMu)` pairs.
    - A **least-squares linear regression** of `Va` vs `Vg1` then yields the slope `dVa/dVg1`, and μ is taken as `μ ≈ -dVa/dVg1` (the standard triode relation at constant Ia). This produces robust, ExtractModel-consistent μ values for use by the Kg1/X/Kp/Kvb estimators and as a seed for pentode fits.
    - See `handoff.md` (section "2025-11-26 Summary (ExtractModel triode μ estimation – working details)") for the full derivation and historical notes (5% vs 50%, regression bug, and sign fix).
  - Export to Devices: after fitting, click "Export to Devices" to save the fitted model preset JSON into `models/` and refresh device dropdowns in Designer. The preset now acts as a **tube-style package**:
    - `model`: fitted parameters (e.g. Gardiner pentode) used by Designer and Modeller.
    - `triodeModel` (optional): embedded Cohen-Helie triode seed used by Modeller for pentode fits when no separate triode model node exists in the project.
    - `measurement` (optional): full analyser sweeps (Va/Vg1/Vg2/Ia/Ig2) embedded in the same JSON, when the export originates from a measurement-based fit.
  - Import from Device: on the Modeller tab, use this button to **pull a measurement back out of a tube-style device preset** (any JSON in `models/` that has an embedded `measurement` block). This clones the sweeps into the current project as a normal Measurement so you can refit models without re-running the analyser, and also selects that Device as the current pentode so subsequent Fits seed from its embedded triodeModel or fitted parameters automatically.
- Designer tab
  - Choose circuit (e.g., Triode Common Cathode)
  - Adjust parameters and see load lines and operating point
  - Export SPICE netlist when ready
  - Device selection: pick a device from the dropdown; if "Show Fitted Model" is checked, red model curves will auto-plot even without measurements
  - For **Single-Ended Output (pentode)**, when the selected device JSON contains an embedded `measurement` block from the analyser, the numeric SE panel now prefers **measurement-based idle**:
    - Given `Vb`, `Vs`, and target `Ia`, Designer interpolates the embedded sweeps near that Va/Vg2 to find `Vg1` and `Ig2`, then reports `Vk = -Vg1`, `Ik = Ia + Ig2`.
    - When no measurement data is present in the preset (legacy or hand-authored JSON), Designer falls back to the model-only bias search used previously.
  - Toggles (bottom row):
    - Show Measurement (measurement curves; when a device preset carries an embedded `measurement`, these sweeps can be displayed directly in Designer as grey curves)
    - Show Fitted Model (model curves; for pentodes, red families step Vg1 in ~2 V increments up to the device's `vg1Max` so the number of model sweeps roughly matches the analyser's grid families)
    - Show Designer Overlays (anode/cathode/AC/OP designer lines)
    - Show Screen Current (pentode only; hidden for triode circuits). This gates Ig2 for both measured sweeps and model overlays in Modeller and Designer.
  - Available Designer circuits (subset wired as of 2025-11-16):
    - Triode Common Cathode (TriodeCC)
    - Pentode Common Cathode (PentodeCC)
    - AC Cathode Follower
    - DC Cathode Follower
    - Single-Ended Output (pentode)
    - Single-Ended UL Output (pentode, UL tap)
    - Push-Pull Output (pentode)
    - Push-Pull UL Output (pentode, UL tap)

### Small-signal & harmonic controls (Modeller / Designer)

- **Modeller μ/gm/ra (mes/mod toggle)**
  - On the Modeller tab, the small-signal μ/gm/ra LCDs can show either **measured** or **modelled** values at a single operating point near 50% of the tube's Ia_max.
  - Use the **mes/mod** checkbox above the LCDs:
    - **Unchecked (measured mode)**: LCDs and labels are **black**. Values are computed from the current measurement using a least-squares estimate over samples near the OP.
    - **Checked (model mode, plain model)**: LCDs and labels turn **red**. Values are computed from the fitted model at the **same** operating point as the measurement, so you can directly compare μ/gm/ra.
    - **Checked (Designer override active)**: when the Designer Triode Common Cathode circuit is active and shares a suitable triode measurement, LCDs and labels turn **green** and show the Designer's μ/gm/ra at its operating point. This keeps Designer and Modeller in sync when you are biasing a triode stage.

- **Designer Triode Common Cathode (TriodeCC)**
  - **Max Sym Swing** checkbox: chooses which helper drives the *Headroom* and μ/gm/ra context when Headroom is zero:
    - Checked: use **symmetric swing** around the OP (blue helper line/zones in the plot).
    - Unchecked: use **maximum swing** into clipping limits (brown helper).
  - **K bypass** checkbox: selects whether cathode bypass is assumed for gain and input-sensitivity calculations:
    - Checked: **bypassed** cathode (higher gain, lower input sensitivity).
    - Unchecked: **unbypassed** cathode (lower gain, higher input sensitivity via local feedback).

- **Designer Single-Ended Output (SE Output)**
  - **Headroom (Vpk)** box:
    - When set to **0.0**, headroom is taken from the plot helpers:
      - Max Sym Swing checked → headroom from symmetric swing around OP.
      - Max Sym Swing unchecked → headroom from maximum possible swing along the AC load line.
    - When set to **> 0**, this manual value overrides the helpers and is used directly for PHEAD, harmonics, and sensitivity.
  - **Max Sym Swing** checkbox: same meaning as TriodeCC; selects whether **symmetric** or **max** swing is used when Headroom is 0. Overlay colours match (blue for symmetric, brown for max).
  - **K bypass** checkbox: sets SE stage gain mode for both **harmonics** and **Input sensitivity (Vpp)**:
    - Checked: cathode **bypassed**, higher gain, lower input sensitivity.
    - Unchecked: cathode **unbypassed**, gain reduced by (1 + gm·Rk), so input sensitivity increases.
  - **Input sensitivity (Vpp)** field below the SE panel:
    - Shows the approximate **signal swing at the input** needed to reach the chosen headroom swing at the anode, using a small-signal gm·Ra estimate and the current K-bypass/headroom settings.
    - Text colour follows the headroom source: bright blue for manual Headroom, light blue for symmetric helper, brown for max-swing helper.
  - **Harmonic metrics (HD2/HD3/HD4/THD):**
    - Both the SE Designer numeric panel and the **Harmonics** tab now use a **sine-driven time-domain simulation**:
      - A pure sine is applied at the grid around the DC bias.
      - For each time sample, the tube model plus AC load line are solved (via `findVaFromVg`) to obtain Va(t) and Ia(t).
      - A Hann-windowed FFT of Ia(t) yields the first few harmonics; HD2/HD3/HD4 and THD are reported as percentages of the fundamental.
    - This allows you to explore harmonic structure from small signal all the way into heavy clipping using the same engine for both Designer and Harmonics plots.

## Preferences (Options → Preferences)

The **Preferences** dialog controls analyser and model behaviour:

- **Analyser Port**
  - Serial port used to talk to the hardware analyser.

- **Pentode Fit**
  - Selects the pentode model family used for Modeller fits (Gardiner, Reefman, Simple Manual Pentode).

- **Sweep spacing**
  - `Linear`: evenly spaced voltage points along analyser sweeps.
  - `Logarithmic`: log-pot style spacing with more points near the pentode knee (low Va) for anode-characteristics tests.

- **Averaging mode / Fixed samples** (analyser auto-averaging)
  - `Averaging mode = Auto`:
    - The analyser chooses the firmware-side current averaging window based on expected max anode current (`iaMax` from the tube template):
      - `iaMax ≤ 5 mA` → 8 samples per point.
      - `5 < iaMax ≤ 30 mA` → 5 samples per point.
      - `iaMax > 30 mA` → 3 samples per point.
  - `Averaging mode = Fixed`:
    - Ignores `iaMax` and always uses the number in **Fixed samples** (1–8) for the firmware averaging window.
  - The active sample count is sent to the firmware via `S0` and reported back as VH in Mode(2) responses (displayed as **Avg per sample** in the analyser UI).

- **Show screen current on anode plots**
  - When checked, Ig2 curves are drawn alongside Ia on analyser and Designer plots (where applicable).
  - When unchecked, only anode current is shown.

- **Pentode remodel after screen modelling**
  - Enables an extra Gardiner pentode "remodel" pass after the screen-current fit.
  - When off, the model is used directly after the main screen fit.

- **Use secondary emissions in model**
  - When checked, the Gardiner pentode model includes secondary-emission shaping parameters (ω, λ, ν) and uses them during fitting and plotting.
  - When unchecked, secondary emission is ignored and those parameters are held out of the solve.

- **Fix triode parameters for pentode modelling**
  - When checked, triode-like parameters in the unified pentode model (e.g. μ, kp, x) are treated as fixed when fitting pentodes (typically taken from a prior triode fit or triode-connected measurement).
  - When unchecked, pentode fits are allowed to adjust those parameters.

- **Fix secondary emission parameters for screen modelling**
  - Only applies when *Use secondary emissions in model* is enabled.
  - When checked, secondary-emission parameters are held fixed during screen-current and remodel stages; when unchecked, they may be adjusted.

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
  - The original uTmax `dr_optimize` code is present under `refrence code/uTmax-master/` for reference only. ValveWorkbench does **not** call `dr_optimize` at runtime; all fitting is done through Ceres Solver and the in-tree Gardiner/Reefman/Cohen–Helie implementations.
- **Simple Manual Pentode** is intended as a manual slider-based modelling path. When the sliders are wired, you will be able to:
  - Select Simple Manual Pentode in Options.
  - Adjust parameters on the Modeller tab and see the model curves move in real time.
  - Save a manually tuned parameter set via "Export to Devices" for use in Designer.
- **Reefman Pentode** is experimental. Future work to align it with ExtractModel_3p0 should be done on a separate branch and validated against that tool’s outputs before it is used for regular fitting.
- Pentode fits now use **centralised parameter bounds** in `Model::setEstimate`, with different envelopes for **Gardiner vs Reefman** models. Gardiner keeps a broad but stable range; Reefman/DerkE uses tighter UTmax-style bounds to keep the DEPIa-style model in a realistic corridor.
- For triode-based seeding of pentode models, the analyser provides a **Triode-Connected Pentode** device type. Measurements taken in this mode appear in the project tree as `Triode (Triode-Connected Pentode) Anode Characteristics` and are used as the triode source for `Estimate::estimatePentode`.
  - Triode seeding typically uses a Cohen–Helie triode fit derived from the triode-connected measurement (or an embedded `triodeModel` in the device JSON). This seed is intended to land the pentode solver in a physically sensible region (µ, Kg1, knee location), not to be a pixel-perfect match to every detail of the triode curves—especially for larger bottles (e.g. 6L6 in triode mode) where small differences in the shallow knee or secondary-emission dip are acceptable as long as overall Ia magnitude and slope are correct.
 - **Pentode vs transfer measurements (behavioural note, 2025‑11‑26):**
   - Pentode model fits (Gardiner/Reefman/Extract DerkE) are driven **only** by pentode **anode-characteristics** measurements in the current project. Pentode **transfer** and **screen** measurements are *not* added to the Ceres solve; they are reserved for diagnostics.
   - When a matching pentode **transfer** measurement exists (Ia vs Vg1 at fixed Va/Vg2), the Modeller small-signal gm display prefers a **least-squares gm from the transfer data at the modelling operating point** (near `Ia ≈ 0.5 × Ia_max`), falling back to anode-based gm when no suitable transfer sweep is available. This affects only the gm/ra/µ LCDs, not the fitted pentode parameters.
   - Known open issue: in at least one 6L6‑GC pentode anode dataset, the **first** Vg1 family in the analyser/Modeller Vg list (e.g. ≈ −20 V) has valid Va/Ia data and appears in the Data tab but its measured curve does not appear in the plot when selected, while other families do. The exact cause has not yet been traced; treat this as a UI/plotting edge case to be investigated with logging or exported measurements before changing code.

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

## Analyser serial protocol (S* and M* commands)

The Analyser tab talks to the Valve Wizard Valve Analyzer over a simple ASCII command
protocol. Commands beginning with `S` set voltages or configuration values on the
hardware; commands beginning with `M` trigger measurement or housekeeping actions.

The implementation of these commands lives in `analyser/analyser.cpp` and the
semantics below are _derived from that code_ (no firmware documentation is
assumed).

### S* commands (setters)

- **`S0 n` – averaging window**  
  Sent once at the start of each test (`startTest()`), where `n` is the number
  of firmware-side samples to average per Mode(2) reading (1–8). The app chooses
  `n` automatically from the tube template’s `iaMax`, or uses the **Fixed
  samples** value from Preferences when Averaging mode is set to `Fixed`.

- **`S2 code` – primary grid DAC (Vg1)**  
  Sets the main control-grid voltage for the first channel. The GUI works in
  volts; the analyser converts that to a firmware DAC code with
  `convertTargetVoltage(GRID, volts)` and wraps it in `S2 <code>`.  
  Used as:
  - Grid **step** command in anode-characteristics tests (`stepCommandPrefix = "S2 "`).
  - Grid **sweep** command in transfer-characteristics tests
    (`sweepCommandPrefix = "S2 "`).
  - Grid reference for both channels in double-triode helpers.

- **`S3 code` – primary anode DAC (Va1)**  
  Sets the main anode (plate) supply. The app uses
  `convertTargetVoltage(ANODE, volts)` to generate `code`.  
  Used as:
  - Anode **sweep** command in anode-characteristics tests
    (`sweepCommandPrefix = "S3 "`).
  - Fixed anode bias in pentode transfer-characteristics tests
    (`sendCommand("S3 <code>")` at test start and when restarting sweeps).  
  During hardware verification the code also uses `S3 0` to pull the anode to 0 V
  before taking a verification sample.

- **`S6 code` – secondary grid DAC (Vg1_B / Vg2_B)**  
  Secondary grid command used for double-triode measurements. In
  `applyGridReferenceBoth()` and double-triode transfer paths, `S6` mirrors
  the primary grid commands (`S2`) to the second channel.

- **`S7 code` – secondary anode / screen supply**  
  Used in two distinct roles:
  - **Secondary anode** in double-triode modes (tracks `S3`).
  - **Screen (Vg2)** in pentode modes. For pentode anode-characteristics tests
    the screen is normally held at `screenStart` and, during verification, is
    temporarily driven to `0 V` via `S7 0`.

### M* commands (measurement / housekeeping)

- **`M1` – discharge / reset between points**  
  Used whenever the analyser needs to discharge coupling capacitors and return
  the hardware to a safe idle before re-applying a new bias. Examples:
  - At the very start of a pentode anode-characteristics test: `S7 0`, `M1`,
    `S3 0`, `M2` to verify the 0 V state.
  - At the first grid point of each sweep in transfer tests, and the first
    anode point in anode tests.
  - When verification fails and the code wants to retry: `M1`, re-assert
    anode/screen, then `M2`.

- **`M2` – Mode(2) measurement sample**  
  Triggers the firmware to take a measurement and return a line beginning with
  `OK: Mode(2)`; `Analyser::checkResponse()` parses these into `Sample`
  objects. `M2` is used both for:
  - **Verification** samples (checking that anode/screen voltages are within
    tolerance before starting a sweep).
  - **Normal** measurement points during a sweep.

- **`M6` – refire / pre-trigger**  
  Used in combination with `M2` on each real sweep point for anode- and
  transfer-characteristics tests:
  - `M6` is sent first to "refire" the hardware at the current bias.
  - `M2` immediately follows to read the resulting anode/screen currents and
    voltages.
  The analyser only sends `M6` when `testType` is ANODE_CHARACTERISTICS or
  TRANSFER_CHARACTERISTICS.

### Typical command flows

These are simplified sequences; see `Analyser::startTest()` and
`Analyser::nextSample()` for exact logic.

- **Pentode anode-characteristics (Va swept, Vg1 stepped, Vg2 fixed)**
  1. `S0 n` – set averaging window.
  2. For first family (first grid step):
     - `S7 0`, `M1`, `S3 0`, `M2` – verify safe 0 V state.
     - On PASS: set screen to `screenStart` and grid to the first step.
  3. For each anode sweep point:
     - `S3 <code>` – new anode voltage.
     - `M6`, `M2` – refire and measure.

- **Pentode transfer-characteristics (Vg1 swept, Vg2 stepped, Va fixed)**
  1. `S0 n` – set averaging window.
  2. `S3 <code>` – set fixed anode voltage once at test start.
  3. For each screen step (family):
     - `S7 <code>` – set screen voltage.
     - For each grid sweep point in that family:
       - `S2 <code>` – grid DAC for Vg1.
       - `M6`, `M2` – refire and measure.

The GUI hides these details; they matter mainly if you are debugging analyser
hardware or extending the serial protocol.

## Troubleshooting
- No “Save to Project” prompt: The app now prompts every time before saving
- Modeller fails to converge: Ensure grid voltages are within range; guard auto‑flips positive Vg to negative
- Empty plots in Designer: Select a device model first
- No model curves in Designer: Ensure a device is selected and "Show Fitted Model" is checked
- Designer overlays extend beyond model curves: Axes are clamped to device `vaMax`/`iaMax`; ensure your device preset has correct limits
- Pentode fits look flatter than datasheets: this is expected with the current Gardiner auto-fit. For precise, datasheet-like curves, the planned Simple Manual Pentode modeller (web-style `epk` backend, manual sliders) will allow you to match curves by hand. Reefman models are experimental and may not converge on all data sets yet.

## Change log (highlights)
- 2025‑11‑18: **Tube-style device JSON**: Export to Devices now optionally embeds the full analyser `measurement` (sweeps) alongside the fitted `model` in a single preset. Designer SE output uses embedded measurement data to compute Vk/Ik/Ig2 when available, with a safe fallback to the fitted tube model for legacy presets.
  - Also: Export to Devices now embeds an optional `triodeModel` seed, Modeller prefers a project triode model or this embedded triodeModel when fitting pentodes, Import from Device selects the corresponding Device so subsequent pentode fits seed from it automatically, and Designer pentode plotting uses a finer grid-family spacing with an explicit Screen Current toggle for Ig2.
- 2025‑11‑26: Triode μ estimation for ExtractModel-style fits:
  - Changed the μ estimation slice in `Estimate::estimateMu` from **5% of Ia_max** to **50% of Ia_max** (with a 1 mA floor) to better match a realistic common operating point for triode-connected pentodes and other power valves.
  - Replaced the previous simple averaging heuristic with a **least-squares linear regression** of `Va` vs `Vg1` at that current slice, inferring μ from the relation `μ ≈ -dVa/dVg1` at constant Ia.
  - Fixed a sign error in the first LS implementation (`μ = slope` instead of `μ = -slope`) which produced incorrect μ values and led to **severely squashed** triode and pentode fits. With the corrected maths and sweep handling, triode-connected 6L6-GC fits now reproduce the expected μ/Kg1 scale and the downstream pentode ExtractModel fits match the known-good Reefman test preset.
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
