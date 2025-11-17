#pragma once

#include "circuit.h"

// Parameters for the push-pull output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum ePushPullParameter {
    PP_VB = 0,   // Supply voltage (V)
    PP_VS,       // Screen voltage (V)
    PP_IA,       // Bias current (anode) (mA)
    PP_RAA,      // Anode-to-anode load (ohms)
    PP_VK,       // Bias point Vk (V)
    PP_IK,       // Total cathode current (mA)
    PP_RK,       // Cathode resistor (ohms)
    PP_POUT      // Max output power (W)
};

class PushPullOutput : public Circuit
{
    Q_OBJECT
public:
    PushPullOutput();

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
