#pragma once

#include "circuit.h"

enum eTriodeCommonCathodeParameter {
    TRI_CC_VB,
    TRI_CC_RK,
    TRI_CC_RA,
    TRI_CC_IA,
    TRI_CC_VK
};

class TriodeCommonCathode : public Circuit
{
public:
    TriodeCommonCathode();

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual void plot(Plot *plot, Device *device);

protected:

    virtual void update(int index);
};
