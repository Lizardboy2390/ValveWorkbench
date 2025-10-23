# Valve Workbench - Calculator Porting Plan

## Overview
This plan outlines the incremental porting of web-based valve calculators from `valvedesigner-web` into the Qt Designer tab. The focus is on small, compileable steps to integrate UI, plots, interfaces, and functionality.

## Current Status
- **Designer Tab Implemented**: Triode Common Cathode calculator basic UI and parameter system functional.
- **Issues Resolved**: Device dropdowns populated, graph display working, no crashes when selecting devices.
- **Model Plotting Fixed**: Grid plot order corrected, loop overrun resolved, debug flood removed.
- **Parameter Scaling Fixed**: Updated 12AX7.json with datasheet parameters, Kg1 internal value 0.000898 appropriate.
- **Next Phase**: Testing and validation of all features.

## Immediate Next Step
- Test all functionality in Modeller and Designer tabs.
- Validate model parameters against datasheet and web version.
- Document fixes and update README.

## Notes
- All changes approved and implemented.
- Work in small steps with user approval and testing.
- Document all changes and test results properly.
