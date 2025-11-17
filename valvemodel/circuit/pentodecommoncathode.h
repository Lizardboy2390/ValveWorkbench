#pragma once

#include "circuit.h"
#include <QVector>

// Parameters for the pentode common-cathode circuit. The first five are
// user-editable inputs; the remaining entries are computed outputs.
enum ePentodeCommonCathodeParameter {
    PENT_CC_VB = 0,   // Supply voltage (V)
    PENT_CC_RK,       // Cathode resistor Rk (Ohms)
    PENT_CC_RA,       // Anode resistor Ra (Ohms)
    PENT_CC_RS,       // Screen resistor Rs (Ohms)
    PENT_CC_RL,       // Load impedance Rl (Ohms)
    PENT_CC_VK,       // Cathode bias Vk (V)
    PENT_CC_VA,       // Anode voltage Va (V)
    PENT_CC_IA,       // Anode current Ia (mA)
    PENT_CC_VG2,      // Screen voltage Vg2 (V)
    PENT_CC_IG2,      // Screen current Ig2 (mA)
    PENT_CC_GM,       // Transconductance gm (mA/V)
    PENT_CC_GAIN,     // Gain (unbypassed)
    PENT_CC_GAIN_B    // Gain (bypassed)
};

class PentodeCommonCathode : public Circuit
{
    Q_OBJECT
public:
    PentodeCommonCathode();

    // UIBridge interface
    void updateUI(QLabel *labels[], QLineEdit *values[]) override;

    // Circuit interface
    void plot(Plot *plot) override;
    int getDeviceType(int index) override;

    // Designer circuits do not currently participate in the project tree, so
    // provide a trivial implementation to satisfy UIBridge's interface.
    QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) override;

protected:
    void update(int index) override;

private:
    void calculateOperatingPoint(double &va, double &ia_mA,
                                 double &vk, double &vg2, double &ig2_mA,
                                 double &gm_mA_V,
                                 double &gain_unbyp, double &gain_byp) const;

    double solveScreenVoltage(double ikTarget_mA,
                              double va, double vg1) const;

    QPointF findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
                                 const QPointF &line2Start, const QPointF &line2End) const;
};
