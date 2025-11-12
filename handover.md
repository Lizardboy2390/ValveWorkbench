# ValveWorkbench Handover

Last updated: 2025-11-12

## Scope of this handover
This note captures what was implemented and verified regarding:
- Grid voltage calibration from Preferences (−5 V and −60 V references to both grids)
- Calibration persistence across runs (QSettings)
- Safe/consistent serial command handling for calibration and test
- Restoring test run behavior (auto-open port on Run)

## Current status
- Test runs successfully from the Run button.
- Preferences grid reference checkboxes work:
  - When idle, S2/S6 commands are written immediately for responsive calibration nudges.
  - When a test is running, commands are sent via the normal pipeline to preserve sequencing.
- Measured calibration entries:
  - Positive values auto-normalize to negative (e.g., 5.005 → −5.005).
  - Clearing a field resets to default (−5.0000 or −60.0000).
- Persistence: values are saved on Options dialog acceptance and reloaded when the dialog is created.

## What changed (files and highlights)
- analyser/analyser.cpp
  - `applyGridReferenceBoth(double, bool)`: Uses Preferences calibration to compute per-grid command voltages; sends S2/S6.
  - Immediate write when idle, else uses `sendCommand` when busy.
- valveworkbench.cpp
  - Connects Preferences grid-ref signal to analyser.
  - Run button now ensures the serial port is open: uses cached port or auto-detects; warns if unavailable and aborts Run.
- preferencesdialog.cpp/h
  - Loads/saves calibration and preferences via QSettings.
  - Measured spin boxes normalize to negative, reset to defaults when cleared, and re-apply output if a ref checkbox is active.

## How to verify
- Test run
  - Click Run; expect no "device not open" writes. If no device, a dialog prompts to select a port.
- Preferences calibration
  - Open Options → toggle −5 V / −60 V. When idle, see immediate S2/S6 writes and voltage changes on hardware.
  - While a test is running, observe commands route through `sendCommand`.
  - Edit measured values: positive entries flip to negative; clear resets to defaults; active ref re-applies immediately.
- Persistence
  - Click OK in Options to save; restart app; values should be restored.

## Design notes
- Calibration mapping is linear between measured low/high (−5 and −60) to compute command voltages for each grid.
- Hardware expects positive DAC codes to generate negative grid potentials; conversions use `convertTargetVoltage(GRID, fabs(v))`.
- Immediate writes are used only when the analyser is idle to avoid disturbing the command pipeline state.

## Known limitations / next candidates
- Auto-save measured values on each change (instead of only on Options OK) if desired.
- Optional setting to disable auto-open on Run (current behavior matches historical expectations).
- Add explicit UI control to open/close port and show port state in Preferences.
- Add logs/UI hints in Preferences when a ref toggle is ignored because the port is closed.

## Quick start for a new engineer/AI
- Entry points:
  - Preferences: `preferencesdialog.cpp/h`
  - Serial/test control: `analyser/analyser.cpp/h`
  - Main window/run path: `valveworkbench.cpp`
- Typical debugging signals:
  - "Preferences applyGridRefRequested"
  - "Grid ref (calibrated)" and "Grid ref (immediate)"
  - Test progress and start/stop logs in analyser and valveworkbench

## Appendix: Acceptance criteria verified
- [x] Run opens the port (cached or auto-detected) and starts test without serial write errors.
- [x] Preferences −5/−60 toggles drive S2/S6 immediately when idle and queue when running.
- [x] Measured values persist and normalize/reset as expected.
