# valvemodel
C++ library for modelling vacuum tubes (valves)

## Overview

This library provides mathematical models and simulation capabilities for vacuum tubes (valves) used in audio and electronic applications. It supports various model types including Cohen-Helie, Koren, and Reefman models for accurate simulation of tube behavior.

## Components

### Models
- **CohenHelieTriode**: Advanced triode model with improved numerical stability
- **KorenTriode**: Norman Koren's triode model implementation
- **SimpleTriode**: Basic triode model for quick calculations
- **ReefmanPentode**: Pentode model based on Reefman's equations
- **GardinerPentode**: Alternative pentode modeling approach

### Circuits
- **TriodeCommonCathode**: Common cathode amplifier circuit
- **TriodeACCathodeFollower**: AC-coupled cathode follower circuit
- **TriodeDCCathodeFollower**: DC-coupled cathode follower circuit
- **PentodeCommonCathode**: Common cathode pentode amplifier circuit

### Data Management
- **Measurement**: Handles measurement data collection and organization
- **Sample**: Individual data point collection
- **Sweep**: Parameter sweep functionality
- **Dataset**: Collection of related measurements

## Recent Improvements

- Enhanced load line calculation methods
- Improved operating point determination
- Better plate curve generation with actual device models
- Numerical stability enhancements in the Cohen-Helie model
- Removed external dependencies (Ceres Solver, ngspice)
- Direct mathematical calculations instead of optimization libraries
- Re-enabled PentodeCommonCathode circuit implementation
- Added defensive programming with safety checks for list access
- Fixed config file loading with absolute path support
- Improved error handling for missing or invalid configuration files
