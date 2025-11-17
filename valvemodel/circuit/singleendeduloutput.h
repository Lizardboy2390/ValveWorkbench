#pragma once

#include "circuit.h"

// Parameters for the single-ended ultralinear output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum eSingleEndedUlParameter {
    SEUL_VB = 0,   // Supply voltage (V)
    SEUL_TAP,      // Screen tap fraction (0..1)
    SEUL_IA,       // Bias current (anode) (mA)
    SEUL_RA,       // Anode load (ohms)
    SEUL_VK,       // Bias point Vk (V)
    SEUL_IK,       // Cathode current (mA)
    SEUL_RK,       // Cathode resistor (ohms)
    SEUL_POUT      // Max output power (W)
};

class SingleEndedUlOutput : public Circuit
{
    Q_OBJECT
public:
    SingleEndedUlOutput();

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
