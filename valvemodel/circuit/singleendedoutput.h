#pragma once

#include "circuit.h"

// Parameters for the single-ended output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum eSingleEndedParameter {
    SE_VB = 0,   // Supply voltage (V)
    SE_VS,       // Screen voltage (V)
    SE_IA,       // Bias current (anode) (mA)
    SE_RA,       // Anode load (ohms)
    SE_VK,       // Bias point Vk (V)
    SE_IK,       // Cathode current (mA)
    SE_RK,       // Cathode resistor (ohms)
    SE_POUT      // Max output power (W)
};

class SingleEndedOutput : public Circuit
{
    Q_OBJECT
public:
    SingleEndedOutput();

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
