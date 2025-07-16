#pragma once

#include "circuit.h"

enum ePentodeCommonCathodeParameter {
    PENT_CC_VB,
    PENT_CC_RK,
    PENT_CC_RA,
    PENT_CC_VK,
    PENT_CC_VA,
    PENT_CC_IA,
    PENT_CC_VG2,
    PENT_CC_IG2,
    PENT_CC_AR,
    PENT_CC_RL,
    PENT_CC_GAIN,
    PENT_CC_GAIN_B
};

class PentodeCommonCathode : public Circuit
{
    Q_OBJECT
public:
    PentodeCommonCathode();

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual void plot(Plot *plot);
    virtual int getDeviceType(int index);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

protected:
    virtual void update(int index);
};
