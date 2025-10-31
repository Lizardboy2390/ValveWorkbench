# Valve Workbench - Calculator Porting Plan

## Overview
This plan outlines the incremental porting of web-based valve calculators from `valvedesigner-web` into the Qt Designer tab. The focus is on small, compileable steps to integrate UI, plots, interfaces, and functionality.

## Current Status
- **Designer Tab Implemented**: Triode Common Cathode calculator basic UI and parameter system functional.
- **Issues Resolved**: Device dropdowns populated, graph display working, no crashes when selecting devices.
- **Model Plotting Fixed**: Grid plot order corrected, loop overrun resolved, debug flood removed.
- **Parameter Scaling Fixed**: Updated 12AX7.json with datasheet parameters, Kg1 internal value 0.000898 appropriate.
- **Grid Calibration UI Added**: Preferences dialog now captures measured −5 V/−60 V grid readings and exposes two-point command helper (analyser integration pending).
- **Double-Triode Sweeps Stabilised**: Selecting measurements or individual sweeps now keeps both triode overlays visible and prevents plot crashes.
- **Modeller Auto Fit**: First visit to the Modeller tab automatically runs one triode fit so both plots appear without manual action.
- **Next Phase**: Testing and validation of all features.

## Immediate Next Step
- Test all functionality in Modeller and Designer tabs.
- Validate model parameters against datasheet and web version.
- Document fixes and update README.
- Plan analyser grid-calibration integration once measurement workflow is verified.
- Design Compare dialog workflow (model selection + parameter display) before implementation.

## Current Objective – Double Triode Modelling
- Run Triode A fitting exactly as today (red curves & parameter column).
- Step 1 ✅: Measurement-layer helper detects second-triode samples (no behaviour change).
- Step 2 ✅: ValveWorkbench helper clones Triode B measurement data (still dormant).
- Step 2a ✅: Keep Triode B clone alive across modelling runs so analyser overlay can render while selecting sweeps.
- Step 3 (pending): Feed cloned measurement into a separate model fit and capture parameters (reintroduce incrementally; current build runs single fit only).
- Step 4: Plot Triode B curves in green and update properties with side-by-side values.
- Keep changes easily revertable and test after each step.

## Pending Feature – Compare Dialog
- ✅ Step 1: Added model-selection panel and reorganised dialog layout to host the upcoming controls.
- ✅ Step 2: Model selectors now populate from available fits, preserve selections, and emit reference/comparison change signals.
- ✅ Step 3: Dialog computes and displays Mu, gm, rp, and Ia for the selected models at the entered test conditions (triode and pentode blocks).
- Present side-by-side values, leaving existing modelling workflow unchanged until feature is ready.

## Notes
- All changes approved and implemented.
- Work in small steps with user approval and testing.
- Document all changes and test results properly.
