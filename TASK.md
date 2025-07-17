# ValveWorkbench Tasks

## Current Tasks

### Fix Calibration and Analyser UI Issues
- [ ] Fix input validation in `checkDoubleValue()` method (incorrect format string `"%f.3"` should be `"%f"` or `"%.3f"`)
- [ ] Add "Step" label to the analyser tab UI to clarify the purpose of the third input field
- [ ] Verify that step values are properly used in the Analyser's measurement sweep logic
- [ ] Ensure calibration values from preferences dialog are correctly applied to measurements
- [ ] Test the full calibration workflow from preferences dialog to measurement conversion
- [ ] Verify all step input fields (anode, grid, screen) are visible and functional in the UI
- [ ] Document the calibration and step value functionality in comments

### Remove Ceres Solver and Fix Build Errors
- [x] Remove Ceres Solver includes and references from source files
- [x] Remove glog and gflags dependencies
- [x] Refactor LinearSolver and QuadraticSolver to use direct calculations
- [x] Remove Ceres, glog, and gflags from ValveWorkbench.pro
- [x] Remove references to missing UI files (comparedialog.ui)
- [x] Replace CompareDialog usage with QMessageBox
- [x] Add pentodecommoncathode.cpp/h files to project file
- [x] Remove ngspice library dependency from ValveWorkbench.pro
- [x] Create stub implementations for ngspice functions
- [x] Fix structure redefinition errors for ValveSample and PentodeSample
- [x] Fix syntax errors in reefmanpentode.cpp
- [x] Fix missing field declarations in residual structs
- [x] Fix missing variables like anodeRemodelOptions in gardinerpentode.cpp
- [x] Ensure consistent struct definitions across all files
- [x] Add proper include guards to prevent redefinition
- [x] Test build after all fixes are implemented
- [x] Verify valve modeling functionality works correctly
- [x] Re-enable PentodeCommonCathode functionality
- [x] Fix QList index out of range errors
- [x] Improve config file loading with absolute paths
- [x] Add safety checks for list access operations

## Discovered During Work

### Structure Definition Issues
- [ ] Decide on a consistent approach for struct definitions (common header vs. local definitions)
- [ ] Fix DerkPentodeResidual and DerkEPentodeResidual structs in reefmanpentode.cpp
- [ ] Ensure proper closure of struct definitions
- [ ] Fix member function definitions being incorrectly placed inside structs

### Build System Issues
- [ ] Update Makefile references to deleted files
- [ ] Run qmake to regenerate build files after changes
- [ ] Address warnings about unused parameters in stub functions

### Calibration and UI Issues
- [ ] Fix input validation for decimal values
- [ ] Ensure consistent behavior across all input fields
- [ ] Verify signal-slot connections for all UI elements

## Feature Functionality Audit

### Analyser Tab
- [ ] Verify all test types function correctly
- [ ] Ensure measurement data is properly displayed in plots
- [ ] Validate heater control functionality
- [ ] Test serial communication with hardware

### Model Tab
- [ ] Verify model fitting functionality
- [ ] Ensure model parameters are correctly displayed
- [ ] Test model export and import features

### Circuit Tab
- [ ] Validate circuit simulation functionality
- [ ] Test parameter adjustment and real-time updates
- [ ] Verify circuit visualization

### Project Management
- [ ] Test project creation, saving, and loading
- [ ] Verify measurement data is properly stored in projects
- [ ] Ensure model data is correctly associated with projects
- [x] Add calibration controls (QDoubleSpinBoxes and QLabel) for heater, anode, screen, and grid voltages and currents to preferences dialog
- [ ] Fix format string in `checkDoubleValue()` method for proper decimal parsing
- [ ] Improve UI layout for step value fields with clear labels
- [ ] Verify the connection between calibration values and measurement conversions

## Completed Tasks

### Dependency Removal
- [x] Identified all Ceres Solver dependencies
- [x] Removed sharedspice.h dependency
- [x] Created ngspice_stubs.cpp with minimal implementations
- [x] Updated ValveWorkbench.pro to remove external dependencies

### Code Refactoring
- [x] Implemented direct mathematical calculations for valve models
- [x] Improved numerical stability based on valvedesigner-web implementation
- [x] Simplified function signatures for compatibility

### Calibration UI Implementation
- [x] Added QDoubleSpinBoxes for all calibration values in preferences dialog
- [x] Connected calibration controls to PreferencesDialog getter/setter methods
- [x] Verified calibration values are transferred between PreferencesDialog and Analyser
