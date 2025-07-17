# ValveWorkbench Project Planning

## Project Architecture

ValveWorkbench is a C++ application for modeling and analyzing vacuum tube (valve) characteristics. The project uses Qt for the GUI and was previously using Ceres Solver for nonlinear optimization, which is now being removed in favor of direct mathematical calculations.

### Core Components

1. **Valve Models**
   - Located in `valvemodel/model/`
   - Implements various mathematical models for different valve types (triodes, pentodes)
   - Key classes: `CohenHelieTriode`, `ReefmanPentode`, `GardinerPentode`, `SimpleTriode`, `KorenTriode`

2. **Circuit Simulation**
   - Located in `valvemodel/circuit/`
   - Previously used ngspice integration, now using simplified stubs
   - Key files: `circuit.h`, `circuit.cpp`, `ngspice_stubs.cpp`

3. **Data Management**
   - Located in `valvemodel/data/`
   - Handles measurement data, projects, and samples
   - Key classes: `Project`, `Measurement`, `Sample`, `Sweep`, `Dataset`

4. **UI Components**
   - Located in `valvemodel/ui/`
   - Provides visualization and user interaction
   - Key classes: `Plot`, `Parameter`, `UIBridge`

5. **Analyzer**
   - Located in `analyser/`
   - Handles hardware communication and measurement
   - Also provides simulation capabilities

## Project Goals

1. ✅ Remove Ceres Solver dependency completely
2. ✅ Replace nonlinear optimization with direct mathematical calculations
3. ✅ Fix build errors and ensure clean compilation
   - ✅ Fixed CompareDialog UI element references
   - ✅ Implemented proper screen current display for pentodes
4. ✅ Maintain valve modeling accuracy and stability
5. ✅ Simplify the codebase and reduce external dependencies
   - ✅ Removed ngspice dependency
   - ✅ Implemented ngspice stubs
6. ✅ Improve error handling and application stability
   - ✅ Added safety checks for list access
   - ✅ Fixed config file loading issues
   - ✅ Re-enabled PentodeCommonCathode functionality
7. 🔄 Implement calibration functionality in the UI
   - ✅ Added calibration controls to preferences dialog
   - ⬜ Test calibration values saving/loading
   - ⬜ Verify calibration values are applied correctly to measurements
8. 🔄 Implement improved valve modeling calculations
   - ✅ Integrated load line calculation methods
   - ✅ Added operating point determination
   - ✅ Enhanced plate curve generation with actual device models
   - ✅ Improved CohenHelieTriode model with numerical stability enhancements
   - ⬜ Complete UI updates to display operating point parameters
9. 🔄 Implement measurement simulation
   - ✅ Added simulation methods
   - ✅ Created UI for the Analyser tab
   - ✅ Implemented different test modes
   - ⬜ Complete progress tracking and visualization for simulations
   - ⬜ Finalize data visualization using QCustomPlot
10. 🔄 Implement simplified project management
    - ✅ Created standalone project management system
    - ✅ Implemented project data storage using internal QString lists
    - ✅ Added JSON serialization/deserialization for project persistence
    - ✅ Built project tree UI
    - ⬜ Complete project item management (add/remove functionality)
11. ⬜ Implement web-based valve model sourcing
    - ⬜ Create web scraping component
    - ⬜ Implement data parser for web data
    - ⬜ Add UI for searching and downloading valve models
    - ⬜ Implement caching system for downloaded models

## Code Style & Conventions

1. **Class Structure**
   - Use Qt's object model with Q_OBJECT macro for signal/slot enabled classes
   - Implement virtual methods for polymorphic behavior
   - Use proper encapsulation with private member variables and public interfaces

2. **Naming Conventions**
   - Classes: PascalCase (e.g., `ValveModel`, `CohenHelieTriode`)
   - Methods: camelCase (e.g., `addSample()`, `calculateParameters()`)
   - Variables: camelCase (e.g., `anodeCurrent`, `gridVoltage`)
   - Constants: ALL_CAPS or k-prefixed PascalCase (e.g., `MAX_VOLTAGE` or `kMaxVoltage`)

3. **File Organization**
   - Header files (.h) contain class declarations
   - Implementation files (.cpp) contain method implementations
   - UI files (.ui) contain Qt Designer layouts
   - Project file (ValveWorkbench.pro) manages build configuration

## Project Constraints

1. Must build with Qt 6.x
2. Must work without Ceres Solver, glog, or gflags
3. Must maintain backward compatibility with existing project files
4. Must provide accurate valve modeling results
5. Should minimize external dependencies

## Development Approach

1. Make incremental changes and test after each modification
2. Fix one issue at a time, prioritizing build errors
3. Maintain consistent structure definitions across files
4. Use proper include guards to prevent redefinition errors
5. Document changes and reasons for modifications

## Detailed Implementation Plan

### Phase 1: Plotting and Visualization (Core Functionality)

1. **Task 1.1: Basic QCustomPlot Integration**
   - Create a basic Plot class wrapper around QCustomPlot
   - Add methods for setting up axes and grid
   - Implement simple curve drawing functionality
   - **Human Testing**: Verify basic plot display with simple data

2. **Task 1.2: Triode Curve Plotting**
   - Implement plate curve generation for triode models
   - Add proper axis labels and scaling
   - Implement curve coloring and styling
   - **Human Testing**: Verify triode curves display correctly

3. **Task 1.3: Pentode Curve Plotting**
   - Extend plotting for pentode characteristics
   - Add screen current visualization
   - Implement proper legend and curve identification
   - **Human Testing**: Verify pentode curves display correctly

4. **Task 1.4: Load Line Visualization**
   - Implement load line calculation and drawing
   - Add operating point marker
   - Implement dynamic updates when parameters change
   - **Human Testing**: Verify load lines and operating points

### Phase 2: Calibration Implementation

1. **Task 2.1: Calibration Data Storage**
   - Implement saving calibration values to config file
   - Add loading of calibration values at startup
   - Create default calibration values
   - **Human Testing**: Verify values persist between application restarts

2. **Task 2.2: Calibration Application**
   - Apply calibration values in Analyser class
   - Implement voltage/current conversion with calibration
   - Add validation for calibration inputs
   - **Human Testing**: Verify calibrated measurements

### Phase 3: Project Management Completion

1. **Task 3.1: Project Item Management**
   - Complete add/remove functionality for project items
   - Implement context menu for project tree
   - Add drag-and-drop support for reorganizing items
   - **Human Testing**: Verify project items can be added/removed/moved

2. **Task 3.2: Project Serialization**
   - Complete JSON serialization for all project data types
   - Implement proper error handling for file operations
   - Add project backup functionality
   - **Human Testing**: Verify projects save and load correctly

3. **Task 3.3: Project Export/Import**
   - Add export functionality for sharing projects
   - Implement import with validation
   - Create project templates
   - **Human Testing**: Verify project export/import works

### Phase 4: Operating Point Calculations

1. **Task 4.1: Operating Point UI**
   - Add UI elements for displaying operating point parameters
   - Implement dynamic updates when circuit parameters change
   - Add power dissipation calculation and warning
   - **Human Testing**: Verify operating point display

2. **Task 4.2: Enhanced Operating Point Visualization**
   - Add visual indicators for operating point on curves
   - Implement "sweet spot" highlighting
   - Add parameter sensitivity analysis
   - **Human Testing**: Verify enhanced visualization

### Phase 5: Measurement Simulation

1. **Task 5.1: Simulation Progress Tracking**
   - Complete progress bar implementation
   - Add time remaining estimation
   - Implement cancelable simulations
   - **Human Testing**: Verify progress tracking

2. **Task 5.2: Simulation Visualization**
   - Complete visualization of simulated data
   - Add comparison between simulated and actual measurements
   - Implement error visualization
   - **Human Testing**: Verify simulation visualization

3. **Task 5.3: Enhanced Simulation**
   - Add realistic noise and variation to simulated data
   - Implement different simulation models
   - Add batch simulation capability
   - **Human Testing**: Verify enhanced simulation

### Phase 6: Web-Based Valve Model Sourcing

1. **Task 6.1: Web Scraping Component**
   - Create web scraping component for valve data
   - Implement data validation and cleaning
   - Add error handling for network issues
   - **Human Testing**: Verify data retrieval from websites

2. **Task 6.2: Data Parser**
   - Implement parser for converting web data to model parameters
   - Add support for different data formats
   - Create parameter estimation for incomplete data
   - **Human Testing**: Verify parameter extraction

3. **Task 6.3: Search UI**
   - Add user interface for searching valve models online
   - Implement filtering and sorting
   - Add preview functionality
   - **Human Testing**: Verify search functionality

4. **Task 6.4: Model Caching**
   - Implement local storage for downloaded models
   - Add cache management (update, delete)
   - Implement offline mode
   - **Human Testing**: Verify caching system

## Current Backup

A backup of the current project state has been created at:
`C:\Users\lizar\Documents\ValveWorkbench_backup_20250717_153249`

This backup can be used to restore the project to its current state if needed during development.
