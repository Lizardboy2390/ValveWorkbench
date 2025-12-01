#pragma once

#include "circuit.h"

// Parameters for the ultralinear push-pull output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum ePushPullUlParameter {
    PPUL_VB = 0,    // Supply voltage (V)
    PPUL_TAP,       // Screen tap fraction (0..1)
    PPUL_IA,        // Bias current (anode) (mA per tube)
    PPUL_RAA,       // Anode-to-anode load (ohms)
    PPUL_HEADROOM,  // Headroom at anode (Vpk)
    PPUL_VK,        // Bias point Vk (V)
    PPUL_IK,        // Cathode current (mA)
    PPUL_RK,        // Cathode resistor (ohms)
    PPUL_POUT,      // Max output power (W)
    PPUL_PHEAD,     // Power at headroom (W)
    PPUL_HD2,       // 2nd harmonic distortion (%)
    PPUL_HD3,       // 3rd harmonic distortion (%)
    PPUL_HD4,       // 4th harmonic distortion (%)
    PPUL_THD        // Total harmonic distortion (%)
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
