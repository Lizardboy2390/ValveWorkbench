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
