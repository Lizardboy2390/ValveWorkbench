# Valve Workbench - Calculator Porting Plan

## Overview
This plan outlines the incremental porting of web-based valve calculators from `valvedesigner-web` into the Qt Designer tab. The focus is on small, compileable steps to integrate UI, plots, interfaces, and functionality.

## Current Status
- **Designer Tab Implemented**: Triode Common Cathode calculator fully ported with UI, plotting, and calculations.
- **Runtime Issue**: Application crashes when selecting "Triode Common Cathode" due to unresolved initialization error.
- **Next Phase**: Debug and fix the runtime crash to ensure stable operation.

## Immediate Next Step
- Debug TriodeCommonCathode circuit selection crash in `valveworkbench.cpp` and `triodecommoncathode.cpp`.
- Identify root cause of initialization failure and propose targeted fix.
- Approval needed for debugging steps and code changes.

## Notes
- Update this document after each completed step.
- All changes must be tiny, approved, and compileable.
