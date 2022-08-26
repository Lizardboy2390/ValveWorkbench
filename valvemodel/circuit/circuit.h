#pragma once

#include "../ui/uibridge.h"
#include "../model/device.h"
#include "../ui/plot.h"
#include "../ui/parameter.h"

enum eCircuitType {
    TRIODE_COMMON_CATHODE,
    PENTODE_COMMON_CATHODE
};

class Circuit : public UIBridge
{
public:
    Circuit();

    virtual void plot(Plot *plot, Device *device) = 0;
    void setParameter(int index, double value);
    double getParameter(int index);

protected:
    Parameter *parameter[8];
    QGraphicsItemGroup *anodeLoadLine = nullptr;
    QGraphicsItemGroup *cathodeLoadLine = nullptr;

    virtual void update(int index) = 0;
};
