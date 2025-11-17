#pragma once

#include "circuit.h"

// Parameters for the single-ended output stage. The first four are
// user-editable inputs; the remaining entries are computed outputs.
enum eSingleEndedParameter {
    SE_VB = 0,   // Supply voltage (V)
    SE_VS,       // Screen voltage (V)
    SE_IA,       // Bias current (anode) (mA)
    SE_RA,       // Anode load (ohms)
    SE_HEADROOM, // Headroom at anode (Vpk)
    SE_VK,       // Bias point Vk (V)
    SE_IK,       // Cathode current (mA)
    SE_RK,       // Cathode resistor (ohms)
    SE_POUT,     // Max output power (W)
    SE_PHEAD,    // Power at headroom (W)
    SE_HD2,      // 2nd harmonic distortion (%)
    SE_HD3,      // 3rd harmonic distortion (%)
    SE_HD4,      // 4th harmonic distortion (%)
    SE_THD       // Total harmonic distortion (%)
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

    // Gain mode for AC calculations: 1 = cathode bypassed (default, higher gain),
    // 0 = unbypassed (lower gain due to local feedback).
    void setGainMode(int mode);
    void setSymSwingEnabled(bool enabled);

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

    bool computeHeadroomPolygonPoints(double vb,
                                      double ia_mA,
                                      double raa,
                                      double headroom,
                                      double vs,
                                      double &vaMin,
                                      double &iaMax,
                                      double &vaMaxDistorted,
                                      double &iaMinDistorted) const;

    void computeHarmonics(double Ia, double Ib, double Ic, double Id, double Ie,
                          double &hd2, double &hd3, double &hd4, double &thd) const;

    bool showSymSwing = true;
    double effectiveHeadroomVpk = 0.0;
    int gainMode = 1;
};
