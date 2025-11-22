#pragma once

#include "circuit.h"

// Parameters for the push-pull output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum ePushPullParameter {
    PP_VB = 0,   // Supply voltage (V)
    PP_VS,       // Screen voltage (V)
    PP_IA,       // Bias current (anode) (mA)
    PP_RAA,      // Anode-to-anode load (ohms)
    PP_HEADROOM, // Headroom at anode (Vpk)
    PP_VK,       // Bias point Vk (V)
    PP_IK,       // Total cathode current (mA)
    PP_RK,       // Cathode resistor (ohms)
    PP_POUT,     // Max output power (W)
    PP_PHEAD,    // Power at headroom (W)
    PP_HD2,      // 2nd harmonic distortion (%)
    PP_HD3,      // 3rd harmonic distortion (%)
    PP_HD4,      // 4th harmonic distortion (%)
    PP_THD       // Total harmonic distortion (%)
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

    // Gain mode for AC calculations: 1 = cathode bypassed (default, higher gain),
    // 0 = unbypassed (lower gain due to local feedback).
    void setGainMode(int mode);

protected:
    void update(int index) override;

private:
    QPointF findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
                                 const QPointF &line2Start, const QPointF &line2End) const;
    double dcLoadlineCurrent(double vb, double raa, double va) const;
    double findGridBiasForCurrent(double targetIa_A,
                                  double vb, double vs, double raa) const;
    double findVaFromVg(double vg1,
                        double vb, double vs, double raa) const;
    bool computeHeadroomHarmonicCurrents(double vb,
                                         double ia_mA,
                                         double raa,
                                         double headroom,
                                         double vs,
                                         double &Ia,
                                         double &Ib,
                                         double &Ic,
                                         double &Id,
                                         double &Ie) const;
    void computeHarmonics(double Ia, double Ib, double Ic, double Id, double Ie,
                          double &hd2, double &hd3, double &hd4, double &thd) const;

    bool simulateHarmonicsTimeDomain(double vb,
                                     double iaBias_mA,
                                     double raa,
                                     double headroomVpk,
                                     double vs,
                                     double &hd2,
                                     double &hd3,
                                     double &hd4,
                                     double &thd) const;

    double effectiveHeadroomVpk = 0.0;
    double inputSensitivityVpp = 0.0;
    int gainMode = 1;
};
