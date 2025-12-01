#pragma once

#include "circuit.h"

// Parameters for the single-ended ultralinear output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum eSingleEndedUlParameter {
    SEUL_VB = 0,    // Supply voltage (V)
    SEUL_TAP,       // Screen tap fraction (0..1)
    SEUL_IA,        // Bias current (anode) (mA)
    SEUL_RA,        // Anode load (ohms)
    SEUL_HEADROOM,  // Headroom at anode (Vpk)
    SEUL_VK,        // Bias point Vk (V)
    SEUL_IK,        // Cathode current (mA)
    SEUL_RK,        // Cathode resistor (ohms)
    SEUL_POUT,      // Max output power (W)
    SEUL_PHEAD,     // Power at headroom (W)
    SEUL_HD2,       // 2nd harmonic distortion (%)
    SEUL_HD3,       // 3rd harmonic distortion (%)
    SEUL_HD4,       // 4th harmonic distortion (%)
    SEUL_THD        // Total harmonic distortion (%)
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

    // Gain mode for AC calculations: 1 = cathode bypassed (default, higher gain),
    // 0 = unbypassed (lower gain due to local feedback).
    void setGainMode(int mode);
    void setInductiveLoad(bool enabled);

protected:
    void update(int index) override;

private:
    QPointF findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
                                 const QPointF &line2Start, const QPointF &line2End) const;

    double dcLoadlineCurrent(double vb, double raa, double va) const;

    double findGridBiasForCurrent(double targetIa_A,
                                  double vb,
                                  double tap,
                                  double raa) const;

    double findVaFromVg(double vg1,
                        double vb,
                        double tap,
                        double raa) const;

    bool computeHeadroomHarmonicCurrents(double vb,
                                         double ia_mA,
                                         double raa,
                                         double headroom,
                                         double tap,
                                         double &Ia,
                                         double &Ib,
                                         double &Ic,
                                         double &Id,
                                         double &Ie) const;

    bool simulateHarmonicsTimeDomain(double vb,
                                     double iaBias_mA,
                                     double raa,
                                     double headroomVpk,
                                     double tap,
                                     double &hd2,
                                     double &hd3,
                                     double &hd4,
                                     double &thd) const;

    double effectiveHeadroomVpk = 0.0;
    double inputSensitivityVpp = 0.0;
    int gainMode = 1;
    bool inductiveLoad = true;
};
