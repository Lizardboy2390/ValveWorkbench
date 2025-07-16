#pragma once

// Common sample structures for valve models
// This file contains shared sample structures used by multiple valve model classes

// Sample structure for storing triode measurement data
struct ValveSample {
    double va;  // Anode voltage
    double vg1; // Grid voltage
    double ia;  // Anode current
};

// Sample structure for storing pentode measurement data
struct PentodeSample {
    double va;   // Anode voltage
    double vg1;  // Grid 1 voltage
    double vg2;  // Grid 2 voltage
    double ia;   // Anode current
    double ig2;  // Screen current
};
