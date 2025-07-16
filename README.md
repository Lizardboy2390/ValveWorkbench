# ValveWorkbench

An engineering tool created with Qt for designing with, modeling and measuring (using the Valve Wizard Valve Analyser) thermionic valves (vacuum tubes).

## Overview

ValveWorkbench provides a comprehensive environment for vacuum tube enthusiasts, engineers, and audio designers to:

- Model various types of vacuum tubes (triodes, pentodes)
- Simulate tube characteristics and behavior
- Measure real tubes using compatible hardware
- Design and analyze tube circuits
- Manage projects and measurement data

## Features

- **Multiple Valve Models**: Includes Cohen-Helie Triode, Koren Triode, Simple Triode, Reefman Pentode, and Gardiner Pentode models
- **Circuit Simulation**: Basic circuit simulation capabilities
- **Measurement Integration**: Works with Valve Wizard Valve Analyser hardware
- **Data Visualization**: Plot tube characteristics and operating points
- **Project Management**: Save, load, and organize your tube data and models

## Dependencies

- Qt 6.x (Core, GUI, PrintSupport, SerialPort, Widgets)
- Eigen3 (for matrix operations)

## Building the Project

1. Ensure Qt 6.x and Eigen3 are installed
2. Open ValveWorkbench.pro in Qt Creator
3. Configure the project for your kit
4. Build the project

## Project Structure

- **analyser/**: Hardware communication and measurement simulation
- **ledindicator/**: LED indicator UI component
- **valvemodel/**: Core valve modeling functionality
  - **circuit/**: Circuit simulation components
  - **data/**: Data management classes
  - **model/**: Valve mathematical models
  - **ui/**: User interface components

## Recent Changes

- Removed dependency on Ceres Solver for nonlinear optimization
- Removed dependency on ngspice library
- Implemented direct mathematical calculations for parameter fitting
- Improved numerical stability in valve models
- Added simulation capabilities for testing without hardware
- Enhanced project management system
- Fixed build errors related to missing files and dependencies
- Re-enabled PentodeCommonCathode functionality
- Added safety checks to prevent QList index out of range errors
- Fixed config file loading issues with absolute paths
- Improved error handling for missing or invalid configuration files

## License

See LICENSE file for details.

