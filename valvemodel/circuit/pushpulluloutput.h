#pragma once

#include "circuit.h"

// Parameters for the ultralinear push-pull output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum ePushPullUlParameter {
    PPUL_VB = 0,   // Supply voltage (V)
    PPUL_TAP,      // Screen tap fraction (0..1)
    PPUL_IA,       // Bias current (anode) (mA)
    PPUL_RAA,      // Anode-to-anode load (ohms)
    PPUL_VK,       // Bias point Vk (V)
    PPUL_IK,       // Cathode current (mA)
    PPUL_RK,       // Cathode resistor (ohms)
    PPUL_POUT      // Max output power (W)
};

class PushPullUlOutput : public Circuit
{
    Q_OBJECT
public:
    PushPullUlOutput();

    void updateUI(QLabel *labels[], QLineEdit *values[]) override;
    void plot(Plot *plot) override;
    int getDeviceType(int index) override;
    QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) override;

protected:
    void update(int index) override;

private:
    QPointF findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
                                 const QPointF &line2Start, const QPointF &line2End) const;
};
