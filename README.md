# Valve Workbench

A comprehensive Qt-based engineering tool for designing, modeling, and measuring thermionic valves (vacuum tubes). This professional-grade application provides complete valve characterization and circuit design capabilities.

## Overview

Valve Workbench is a sophisticated vacuum tube engineering application that combines:
- **Hardware Control**: Real-time measurement using the Valve Wizard Valve Analyzer
- **Curve Fitting**: Professional-grade mathematical modeling with Ceres Solver optimization
- **Circuit Design**: Interactive circuit configuration and analysis
- **SPICE Export**: Generate simulation-ready netlists for circuit validation

## Features

### ✅ **Fully Functional Components**

#### **Analyser Tab (Hardware Control)**
- Serial communication with Valve Wizard Valve Analyzer hardware
- Real-time voltage/current measurement and conversion
- ADC value conversion to physical units (voltage/current scaling)
- Command buffering and timeout handling
- Safety features: hardware verification, current/power limiting
- Automated measurement sequences with progress tracking

#### **Modeller Tab (Curve Fitting & Modeling)**
- **Professional-grade curve fitting** using Ceres Solver optimization
- **Multiple model types**:
  - **Triode Models**: Simple Triode, Koren Triode, Cohen-Helie Triode
  - **Pentode Models**: Gardiner Pentode, Reefman Pentode (Derk & E variants)
- **Sophisticated mathematical modeling** of vacuum tube characteristics
- **Red curve plotting** for fitted models vs measured data
- **Parameter estimation** with 23+ model parameters
- **Threaded execution** for smooth UI experience

#### **Designer Tab (Circuit Configuration)**
- **Circuit type selection** and device configuration
- **16 configurable circuit parameters** (supply voltage, resistors, loads, etc.)
- **Device selection** (Triode, Pentode, Double Triode, Diode)
- **Interactive load line calculations** and operating point analysis
- **Real-time plotting** of anode curves, load lines, and operating points
- **Gain calculations** (bypassed and unbypassed cathode configurations)
- **SPICE export** functionality for circuit simulation

## Technical Architecture

### **Backend**
- **C++ with Qt Framework** for cross-platform compatibility
- **Ceres Solver** for non-linear least squares optimization
- **Sophisticated mathematical models** for vacuum tube characteristics

### **Hardware Interface**
- **Serial communication** with custom analyzer hardware
- **Real-time data acquisition** and processing
- **Safety protocols** and error handling

### **User Interface**
- **Three-tab interface**: Designer, Modeller, Analyser
- **Interactive plotting** with multiple curve types
- **Real-time parameter updates** and calculations
- **Professional data visualization**

## Project Structure

```
ValveWorkbench/
├── analyser/           # Hardware control and measurement
├── valvmodel/          # Mathematical modeling and curve fitting
├── circuits/           # Circuit design and analysis
├── docs/              # Generated documentation
├── models/            # Device model files
├── Spice64/           # SPICE simulation integration
└── [UI components]    # Qt interface files
```

## Current Development Status

### ✅ **COMPLETED & FULLY FUNCTIONAL**
- **Analyser Tab**: Complete hardware control and measurement ✅
- **Modeller Tab**: Complete curve fitting and modeling ✅
- **Designer Tab**: Complete circuit configuration and analysis ✅
- **UI Integration**: Complete with real-time updates ✅
- **Mathematical Models**: Professional-grade with multiple types ✅
- **Hardware Interface**: Complete serial communication system ✅

### **Key Technical Achievements**
- **Professional valve modeling** with Ceres optimization
- **Real-time hardware control** and measurement
- **Interactive circuit design** with load line analysis
- **SPICE export** for circuit simulation
- **Threaded execution** for smooth UI experience
- **Comprehensive error handling** and safety features

## Installation & Usage

### **Prerequisites**
- Qt 6.9+ development environment
- Ceres Solver library
- Valve Wizard Valve Analyzer hardware (for measurement features)

### **Building**
```bash
qmake
make
```

### **Running**
```bash
./ValveWorkbench
```

## Hardware Integration

The application integrates with the **Valve Wizard Valve Analyzer** for real-time measurement:
- Automated heater control and monitoring
- Precision voltage/current measurement
- Safety limits and hardware verification
- Multiple test configurations (anode characteristics, transfer characteristics)

## Circuit Design Features

### **Supported Circuit Types**
- **Triode Common Cathode** (fully implemented)
- Extensible framework for additional circuit types

### **Design Capabilities**
- Interactive parameter adjustment
- Real-time load line visualization
- Operating point calculation
- Gain analysis (bypassed/unbypassed)
- SPICE netlist generation

## Mathematical Modeling

### **Curve Fitting Engine**
- **Ceres Solver integration** for robust optimization
- **Multiple model families** for different valve types
- **Parameter estimation** with statistical analysis
- **Residual analysis** for model validation

### **Model Types**
- **Simple Triode**: Basic three-parameter model
- **Koren Triode**: Enhanced accuracy model
- **Cohen-Helie Triode**: Research-grade model
- **Gardiner Pentode**: Multi-section pentode model
- **Reefman Pentode**: Alternative pentode formulation

## License

This project includes components from various sources. See `included-licences/` directory for detailed license information.

## Contributing

The application is designed with a modular architecture allowing for easy extension:
- **Circuit types** can be added by implementing the Circuit interface
- **Device models** can be added through the Device class
- **Plotting** uses a flexible Plot class for visualization
- **Hardware interfaces** can be extended for different analyzer types

## Support

For technical support or questions about valve modeling and circuit design, refer to:
- Project documentation in `docs/`
- User manual: `manual.docx`
- Source code comments and inline documentation

---

**Status**: Production-ready vacuum tube characterization and modeling software with professional-grade features and hardware integration.
