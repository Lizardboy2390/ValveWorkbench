#pragma once

enum eDevice {
    PENTODE,
    TRIODE,
    DOUBLE_TRIODE,
    DIODE
};

enum ePentode {
    PNT_OTHER_DEVICE,
    PNT_TRUE_PENTODE,
    PNT_BEAM_TETRODE,
    PNT_TRUE_PENTODE_SE,
    PNT_BEAM_TETRODE_SE
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
