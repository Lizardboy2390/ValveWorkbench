#pragma once

#include "../ui/uibridge.h"
#include "../model/device.h"
#include "../ui/plot.h"
#include "../ui/parameter.h"

#include "sharedspice.h"

#include <tchar.h>
#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>
#endif

enum eCircuitType {
    TRIODE_COMMON_CATHODE,
    PENTODE_COMMON_CATHODE,
    AC_CATHODE_FOLLOWER,
    DC_CATHODE_FOLLOWER,
    LONG_TAILED_PAIR,
    CATHODYNE_PHASE_SPLITTER,
    SINGLE_ENDED_OUTPUT,
    ULTRALINEAR_SINGLE_ENDED,
    PUSH_PULL_OUTPUT,
    ULTRALINEAR_PUSH_PULL,
    TEST_CALCULATOR  // New test item
};

class Circuit : public UIBridge
{
    Q_OBJECT
public:
    Circuit();

    virtual void plot(Plot *plot) = 0;
    virtual int getDeviceType(int index) = 0;

    void setOverlaysVisible(bool visible);

    // Drop references to any overlay groups (load lines, OP markers) without
    // touching the scene. This is used when the shared Plot scene is cleared
    // externally (e.g., when switching circuits) to avoid dangling pointers.
    void resetOverlays();

    void setParameter(int index, double value);
    double getParameter(int index);

    void setDevice1(Device *newDevice1);

    void setDevice2(Device *newDevice2);

protected:
    Parameter *parameter[16];
    QGraphicsItemGroup *anodeLoadLine = nullptr;
    QGraphicsItemGroup *cathodeLoadLine = nullptr;
    QGraphicsItemGroup *acSignalLine = nullptr;
    QGraphicsItemGroup *opMarker = nullptr;

    Device *device1;
    Device *device2;

    virtual void update(int index) = 0;

    bool waitSimulationEnd();
};
