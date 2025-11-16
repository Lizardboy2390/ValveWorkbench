#pragma once

#include "circuit.h"

enum eTriodeACCathodeFollowerParameter {
    TRI_ACCF_VB,
    TRI_ACCF_RB,
    TRI_ACCF_RK,
    TRI_ACCF_RL,
    TRI_ACCF_VG,
    TRI_ACCF_VK,
    TRI_ACCF_IK,
    TRI_ACCF_RO
};

#include <QPointF>
#include <QVector>
#include <cmath>

class TriodeACCathodeFollower : public Circuit
{
    Q_OBJECT

public:
    TriodeACCathodeFollower();

    virtual void updateUI(QLabel *labels[], QLineEdit *values[]) override;
    virtual void plot(Plot *plot) override;
    virtual int getDeviceType(int index) override;

    // Designer circuits do not currently participate in the project tree, so
    // provide a trivial implementation to satisfy UIBridge's interface.
    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) override;

protected:
    virtual void update(int index) override;

private:
    QVector<QPointF> biasLoadLineData;
    QVector<QPointF> cathodeLoadLineData;

    void calculateLoadLines();
    QPointF findLineIntersection(QPointF line1Start, QPointF line1End,
                                 QPointF line2Start, QPointF line2End) const;
    QPointF findOperatingPoint();
    void computeSmallSignal(double va_k, double ia_mA,
                            double &vg, double &vk,
                            double &gm_mA_V, double &ro_ohm) const;
};
