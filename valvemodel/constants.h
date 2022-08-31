#pragma once

enum eDevice {
    PENTODE,
    TRIODE,
    DOUBLE_TRIODE,
    DIODE
};

enum eTest {
    ANODE_CHARACTERISTICS,
    SCREEN_CHARACTERISTICS,
    TRANSFER_CHARACTERISTICS
};

enum eElectrode {
    HEATER,
    ANODE,
    GRID,
    SCREEN
};

enum eItemType {
    TYP_PROJECT,
    TYP_MEASUREMENT,
    TYP_SWEEP,
    TYP_SAMPLE,
    TYP_ESTIMATE,
    TYP_MODEL
};
