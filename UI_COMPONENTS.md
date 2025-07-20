# ValveWorkbench UI Components

This document provides an overview of the main UI components in ValveWorkbench and their functionality.

## Main Tabs

ValveWorkbench is organized into three main tabs:

1. **Designer Tab**: For designing and simulating valve circuits
2. **Modeller Tab**: For managing projects and fitting models to measurement data
3. **Analyser Tab**: For measuring real valve characteristics using hardware

## Analyser Tab

The Analyser tab provides an interface for measuring valve characteristics using compatible hardware.

### Device Selection

The Analyser tab allows testing different types of valves:

- **Triode A**: For testing single triodes or the first half of a double triode
- **Triode B**: For testing the second half of a double triode
- **Pentode**: For testing pentode valves

When working with double triodes (such as 12AU7, 6SN7, etc.), you can select either "Triode A" or "Triode B" to test each half individually. This is particularly useful for matching sections of a double triode or identifying differences between the two halves.

### Test Types

The Analyser supports different test types depending on the selected device:

- **Anode Characteristics**: Measures anode current vs. anode voltage at different grid voltages
- **Transfer Characteristics**: Measures anode current vs. grid voltage at different anode voltages
- **Screen Characteristics** (Pentodes only): Measures screen current vs. screen voltage

### Measurement Parameters

For each test, you can configure:

- **Heater Voltage**: Set the appropriate heater voltage for the valve being tested
- **Anode Voltage**: Configure start, stop, and step values for the anode voltage sweep
- **Grid Voltage**: Configure start, stop, and step values for the grid voltage sweep
- **Screen Voltage** (Pentodes only): Configure start, stop, and step values for the screen voltage

### Templates

The Analyser tab allows saving and loading test templates to quickly set up common test configurations.

## Designer Tab

The Designer tab provides tools for designing and simulating valve circuits.

[Designer tab details would go here]

## Modeller Tab

The Modeller tab provides tools for managing projects and fitting models to measurement data.

[Modeller tab details would go here]
