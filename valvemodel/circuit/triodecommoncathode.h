#pragma once

#include "circuit.h"

enum eTriodeCommonCathodeParameter {
    TRI_CC_VB,
    TRI_CC_RK,
    TRI_CC_RA,
    TRI_CC_RL,
    TRI_CC_VK,
    TRI_CC_VA,
    TRI_CC_IA,
    TRI_CC_AR,
    TRI_CC_GAIN,
    TRI_CC_GAIN_B,
    TRI_CC_MU,
    TRI_CC_GM
};

#include <QPointF>
#include <QVector>
#include <cmath>
#include <QJsonObject>

class TriodeCommonCathode : public Circuit
{
    Q_OBJECT
public:
    TriodeCommonCathode();

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual void plot(Plot *plot);
    virtual int getDeviceType(int index);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

    // SPICE export functionality
    QJsonObject exportSPICE();

protected:
    virtual void update(int index);

private:
    // Load line data storage
    QVector<QPointF> anodeLoadLineData;
    QVector<QPointF> cathodeLoadLineData;

    // Annotations for maximum voltage swing (vertical line and labels)
    QGraphicsItemGroup *swingGroup = nullptr;

    // Maximum dissipation line
    QGraphicsItemGroup *paLimitGroup = nullptr;

    // Symmetrical swing helper and input sensitivity overlays
    QGraphicsItemGroup *symSwingGroup = nullptr;
    QGraphicsItemGroup *sensitivityGroup = nullptr;

    // Overlay toggles and gain mode: 0 = unbypassed, 1 = bypassed
    bool showSymSwing = true;
    bool showInputSensitivity = true;
    int sensitivityGainMode = 1;

    // Cached symmetric output swing (Vpp) computed during plot
    double lastSymVpp = 0.0;

    // Circuit calculation methods
    void calculateAnodeLoadLine();
    void calculateCathodeLoadLine();
    QPointF findOperatingPoint();
    QPointF findOperatingPointSimple();
    QPointF findLineIntersection(QPointF line1Start, QPointF line1End,
                                QPointF line2Start, QPointF line2End);
    void calculateOperatingPoint();
    void calculateGains();

public:
    void setSymSwingEnabled(bool enabled) { showSymSwing = enabled; }
    void setInputSensitivityEnabled(bool enabled) { showInputSensitivity = enabled; }
    void setSensitivityGainMode(int mode) { sensitivityGainMode = mode; }
    void setDesignerOverlaysVisible(bool visible) {
        if (anodeLoadLine) anodeLoadLine->setVisible(visible);
        if (cathodeLoadLine) cathodeLoadLine->setVisible(visible);
        if (acSignalLine) acSignalLine->setVisible(visible);
        if (opMarker) opMarker->setVisible(visible);
        if (swingGroup) swingGroup->setVisible(visible);
        if (paLimitGroup) paLimitGroup->setVisible(visible);
        if (symSwingGroup) symSwingGroup->setVisible(visible);
        if (sensitivityGroup) sensitivityGroup->setVisible(visible);
    }
};
