#pragma once

#include "circuit.h"

class TriodeACCathodeFollower : public Circuit
{
    Q_OBJECT
public:
    TriodeACCathodeFollower();

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual void plot(Plot *plot);
    virtual int getDeviceType(int index);
    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

protected:
    virtual void update(int index);

private:
    enum eParam {
        ACCF_VB = 0,
        ACCF_RK,
        ACCF_RA,
        ACCF_RL,
        ACCF_RG,    // new input: grid resistor (Ohms)
        ACCF_VK,
        ACCF_VA,
        ACCF_IA,
        ACCF_RA_INT,
        ACCF_GAIN,
        ACCF_GAIN_B,
        ACCF_MU,
        ACCF_GM,
        ACCF_ZIN,
        ACCF_ZO
    };

    // Annotations
    QGraphicsItemGroup *swingGroup = nullptr;
    QGraphicsItemGroup *paLimitGroup = nullptr;
    QGraphicsItemGroup *symSwingGroup = nullptr;
    QGraphicsItemGroup *sensitivityGroup = nullptr;

    // Toggles and cache
    bool showSymSwing = true;
    int sensitivityGainMode = 1; // 0=unbypassed,1=bypassed (for consistency)
    double lastSymVpp = 0.0;

    // Helpers
    void calcAnodeLine(QVector<QPointF> &out, double vb, double ra, double rk, double xStop, double yStop);
    void calcCathodeLine(QVector<QPointF> &out, double rk, double xStop, double yStop);
    QPointF findIntersection(const QVector<QPointF> &a, const QVector<QPointF> &b);

public:
    void setSymSwingEnabled(bool enabled) { showSymSwing = enabled; }
    void setSensitivityGainMode(int mode) { sensitivityGainMode = mode; }
};
