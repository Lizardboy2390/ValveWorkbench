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

    // Debug helper: scan time-domain THD vs headroom into clipping and log
    // results to the application output. Intended for manual use only.
    void debugScanHeadroomTimeDomain() const;

    // Helper for the Harmonics tab: compute a time-domain THD scan over
    // headroom and return the sampled headroom values and harmonic contents
    // so that the caller can graph operating point vs harmonic content.
    void computeTimeDomainHarmonicScan(QVector<double> &headroomVals,
                                       QVector<double> &hd2Vals,
                                       QVector<double> &hd3Vals,
                                       QVector<double> &hd4Vals,
                                       QVector<double> &thdVals) const;

    // Helper for the Harmonics tab: compute a bias-sweep harmonic curve over
    // a range of bias currents Ia at a fixed headroom, returning the sampled
    // Ia values and harmonic contents so that the caller can graph harmonic
    // level vs bias current.
    void computeBiasSweepHarmonicCurve(QVector<double> &iaVals,
                                       QVector<double> &hd2Vals,
                                       QVector<double> &hd3Vals,
                                       QVector<double> &hd4Vals,
                                       QVector<double> &hd5Vals,
                                       QVector<double> &thdVals) const;

    void computeHarmonicHeatmapData(QVector<double> &operatingPoints,
                                   QVector<QVector<double>> &harmonicMatrix,
                                   bool sweepBias = true) const;

    void computeHarmonicSurfaceData(QVector<double> &biasPoints,
                                   QVector<double> &headroomPoints,
                                   QVector<QVector<QVector<double>>> &harmonicSurface) const;

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

    bool simulateHarmonicsTimeDomain(double vb,
                                     double iaBias_mA,
                                     double raa,
                                     double headroomVpk,
                                     double vs,
                                     double &hd2,
                                     double &hd3,
                                     double &hd4,
                                     double &hd5,
                                     double &thd) const;

    bool showSymSwing = true;
    double effectiveHeadroomVpk = 0.0;
    int gainMode = 1;
};
