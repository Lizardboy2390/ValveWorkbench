#include "singleendedoutput.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <QGraphicsPolygonItem>

#include <algorithm>
#include <cmath>
#include <limits>

SingleEndedOutput::SingleEndedOutput()
{
    // Input parameters mirroring web SingleEnded defaults
    parameter[SE_VB]       = new Parameter("Supply voltage (V)", 300.0);
    parameter[SE_VS]       = new Parameter("Screen voltage (V)", 250.0);
    parameter[SE_IA]       = new Parameter("Bias current (anode) (mA)", 30.0);
    parameter[SE_RA]       = new Parameter("Anode load (\u03a9)", 8000.0);
    parameter[SE_HEADROOM] = new Parameter("Headroom at anode (Vpk)", 0.0);

    // Calculated values
    parameter[SE_VK]   = new Parameter("Bias point Vk (V)", 0.0);
    parameter[SE_IK]   = new Parameter("Cathode current (mA)", 0.0);
    parameter[SE_RK]   = new Parameter("Cathode resistor (\u03a9)", 0.0);
    parameter[SE_POUT] = new Parameter("Max output power (W)", 0.0);
    parameter[SE_PHEAD]= new Parameter("Power at headroom (W)", 0.0);
    parameter[SE_HD2]  = new Parameter("2nd harmonic (%)", 0.0);
    parameter[SE_HD3]  = new Parameter("3rd harmonic (%)", 0.0);
    parameter[SE_HD4]  = new Parameter("4th harmonic (%)", 0.0);
    parameter[SE_THD]  = new Parameter("Total harmonic (%)", 0.0);
}

void SingleEndedOutput::setGainMode(int mode)
{
    gainMode = mode ? 1 : 0;

    // Changing gain mode affects THD scaling for a given headroom setting.
    // Re-run update() so that SE_HD2/3/4/THD reflect the new mode. Use
    // SE_HEADROOM as the index since that parameter drives the headroom
    // and distortion calculations.
    update(SE_HEADROOM);
}

void SingleEndedOutput::setSymSwingEnabled(bool enabled)
{
    showSymSwing = enabled;
    // Changing which swing helper we treat as the default when Headroom==0
    // affects the effectiveHeadroomVpk used for PHEAD/THD/sensitivity, so
    // recompute using SE_HEADROOM as the driver.
    update(SE_HEADROOM);
}

int SingleEndedOutput::getDeviceType(int index)
{
    Q_UNUSED(index);
    // Prefer pentode devices for SE output stage
    return PENTODE;
}

QTreeWidgetItem *SingleEndedOutput::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void SingleEndedOutput::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first parameters up to SE_HEADROOM
    for (int i = 0; i <= SE_HEADROOM; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);

            if (i == SE_HEADROOM) {
                const double headroomManual = parameter[SE_HEADROOM]->getValue();
                const bool overrideActive   = (headroomManual > 0.0);

                QString style;
                if (overrideActive) {
                    // Manual override of helper swings: bright blue.
                    style = "color: rgb(0,0,255);";
                } else if (effectiveHeadroomVpk > 0.0) {
                    // Headroom derived from helpers: lighter blue for symmetric
                    // mode, brown for max-swing mode.
                    if (showSymSwing) {
                        style = "color: rgb(100,149,237);"; // cornflower blue
                    } else {
                        style = "color: rgb(165,42,42);";
                    }
                }
                labels[i]->setStyleSheet(style);
                values[i]->setStyleSheet(style);
            } else {
                labels[i]->setStyleSheet("");
                values[i]->setStyleSheet("");
            }
        }
    }

    const double headroomManual = parameter[SE_HEADROOM]->getValue();
    const bool overrideActive   = (headroomManual > 0.0);

    // Outputs: remaining fields after SE_HEADROOM
    for (int i = SE_VK; i <= SE_THD; ++i) {
        if (!labels[i] || !values[i]) continue;

        QString labelText;
        switch (i) {
        case SE_VK:   labelText = "Bias point Vk (V):"; break;
        case SE_IK:   labelText = "Cathode current (mA):"; break;
        case SE_RK:   labelText = "Cathode resistor (\u03a9):"; break;
        case SE_POUT: labelText = "Max output power (W):"; break;
        case SE_PHEAD:labelText = "Power at headroom (W):"; break;
        case SE_HD2:  labelText = "2nd harmonic (%):"; break;
        case SE_HD3:  labelText = "3rd harmonic (%):"; break;
        case SE_HD4:  labelText = "4th harmonic (%):"; break;
        case SE_THD:  labelText = "Total harmonic (%):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = 3;
            switch (i) {
            case SE_VK:
            case SE_IK:
            case SE_RK:
                decimals = 3; break;
            case SE_POUT:
            case SE_PHEAD:
            case SE_HD2:
            case SE_HD3:
            case SE_HD4:
            case SE_THD:
                decimals = 2; break;
            default: break;
            }
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);

        // Color distortion-related outputs. When Headroom>0, metrics are based
        // on the manual override and use bright blue. When Headroom==0 and
        // helpers drive the effective headroom, use lighter blue for symmetric
        // mode (Max Sym Swing on) and brown for max-swing mode (Max Sym Swing off).
        if (i == SE_PHEAD || i == SE_HD2 || i == SE_HD3 || i == SE_HD4 || i == SE_THD) {
            if (effectiveHeadroomVpk > 0.0) {
                if (overrideActive) {
                    // Manual override: bright blue
                    values[i]->setStyleSheet("color: rgb(0,0,255);");
                } else if (showSymSwing) {
                    // Symmetric-mode metrics: lighter blue
                    values[i]->setStyleSheet("color: rgb(100,149,237);");
                } else {
                    // Max-swing metrics: brown
                    values[i]->setStyleSheet("color: rgb(165,42,42);");
                }
            } else {
                values[i]->setStyleSheet("");
            }
        } else {
            values[i]->setStyleSheet("");
        }
    }

    const int sensIndex = SE_THD + 1;
    if (sensIndex < 16 && labels[sensIndex] && values[sensIndex]) {
        labels[sensIndex]->setText("Input sensitivity (Vpp):");

        // Derive sensitivity from the effective headroom (Vpk) so that both
        // distortion and sensitivity refer to the same test swing. When the
        // manual headroom is zero, effectiveHeadroomVpk is taken from either
        // the symmetric swing helper (if showSymSwing is true) or from the
        // max swing helper.
        double vppIn = 0.0;
        if (device1) {
            const double headroom = effectiveHeadroomVpk; // Vpk at anode
            const double vb       = parameter[SE_VB]->getValue();
            const double vs       = parameter[SE_VS]->getValue();
            const double raa      = parameter[SE_RA]->getValue();
            const double vk       = parameter[SE_VK]->getValue();
            const double rk       = parameter[SE_RK]->getValue();

            if (effectiveHeadroomVpk > 0.0 && raa > 0.0) {
                const double Vpp = 2.0 * headroom; // peak-to-peak swing at anode

                const double vgBias = -vk; // grid-to-cathode bias
                const double dVg    = std::max(0.05, std::abs(vgBias) * 0.02);

                double iaPlus  = device1->anodeCurrent(vb, vgBias + dVg, vs);
                double iaMinus = device1->anodeCurrent(vb, vgBias - dVg, vs);
                double gm_mA_per_V = 0.0;
                if (std::isfinite(iaPlus) && std::isfinite(iaMinus) && dVg > 0.0) {
                    gm_mA_per_V = (iaPlus - iaMinus) * 1000.0 / (2.0 * dVg);
                }

                double gain = 0.0;
                if (std::isfinite(gm_mA_per_V) && raa > 0.0) {
                    // Base small-signal gain for bypassed cathode: Av ≈ gm(A/V) * Ra.
                    const double gm_A_per_V = gm_mA_per_V / 1000.0;
                    gain = std::abs(gm_A_per_V * raa);

                    // If cathode is unbypassed (gainMode == 0), include local
                    // feedback from Rk: effective gain ≈ Av / (1 + gm*Rk).
                    if (gainMode == 0 && rk > 0.0) {
                        const double feedback = 1.0 + gm_A_per_V * rk;
                        if (feedback > 1.0 && std::isfinite(feedback)) {
                            gain /= feedback;
                        }
                    }
                }

                if (std::isfinite(gain) && gain > 1e-6) {
                    vppIn = Vpp / gain;
                }
            }
        }

        if (vppIn > 0.0) {
            values[sensIndex]->setText(QString::number(vppIn, 'f', 2));
        } else {
            values[sensIndex]->setText("");
        }

        labels[sensIndex]->setVisible(true);
        values[sensIndex]->setVisible(true);
        values[sensIndex]->setReadOnly(true);

        // Color sensitivity to match the headroom source: bright blue when
        // manual Headroom>0, otherwise lighter blue for symmetric mode and
        // brown for max-swing mode.
        if (effectiveHeadroomVpk > 0.0) {
            if (overrideActive) {
                values[sensIndex]->setStyleSheet("color: rgb(0,0,255);");
            } else if (showSymSwing) {
                values[sensIndex]->setStyleSheet("color: rgb(100,149,237);");
            } else {
                values[sensIndex]->setStyleSheet("color: rgb(165,42,42);");
            }
        } else {
            values[sensIndex]->setStyleSheet("");
        }
    }

    // Hide remaining parameter slots
    for (int i = sensIndex + 1; i < 16; ++i) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
        }
    }
}

void SingleEndedOutput::computeBiasSweepHarmonicCurve(QVector<double> &iaVals,
                                                      QVector<double> &hd2Vals,
                                                      QVector<double> &hd3Vals,
                                                      QVector<double> &hd4Vals,
                                                      QVector<double> &hd5Vals,
                                                      QVector<double> &thdVals) const
{
    iaVals.clear();
    hd2Vals.clear();
    hd3Vals.clear();
    hd4Vals.clear();
    hd5Vals.clear();
    thdVals.clear();

    const double vb       = parameter[SE_VB]->getValue();
    const double vs       = parameter[SE_VS]->getValue();
    const double iaCenter = parameter[SE_IA]->getValue();
    const double raa      = parameter[SE_RA]->getValue();
    const double headroom = parameter[SE_HEADROOM]->getValue();

    if (!device1 || vb <= 0.0 || raa <= 0.0 || iaCenter <= 0.0 || headroom <= 0.0) {
        return;
    }

    // Sweep bias current around the current operating point to explore how
    // harmonic content changes with bias. Use a modest range and step count
    // to keep the helper inexpensive.
    const double iaMin = std::max(iaCenter * 0.5, 1.0);            // mA
    const double iaMax = std::min(iaCenter * 1.5, device1->getIaMax());
    if (iaMax <= iaMin) {
        return;
    }

    const int steps = 32;
    iaVals.reserve(steps);
    hd2Vals.reserve(steps);
    hd3Vals.reserve(steps);
    hd4Vals.reserve(steps);
    hd5Vals.reserve(steps);
    thdVals.reserve(steps);

    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(steps);
        const double iaBias = iaMin + (iaMax - iaMin) * t; // mA

        double hd2 = 0.0;
        double hd3 = 0.0;
        double hd4 = 0.0;
        double hd5 = 0.0;
        double thd = 0.0;
        if (simulateHarmonicsTimeDomain(vb, iaBias, raa, headroom, vs, hd2, hd3, hd4, hd5, thd)) {
            iaVals.push_back(iaBias);
            hd2Vals.push_back(hd2);
            hd3Vals.push_back(hd3);
            hd4Vals.push_back(hd4);
            hd5Vals.push_back(hd5);
            thdVals.push_back(thd);
        }
    }
}

void SingleEndedOutput::computeHarmonicHeatmapData(QVector<double> &operatingPoints,
                                                   QVector<QVector<double>> &harmonicMatrix,
                                                   bool sweepBias) const
{
    operatingPoints.clear();
    harmonicMatrix.clear();

    const double vb = parameter[SE_VB]->getValue();
    const double vs = parameter[SE_VS]->getValue();
    const double ia = parameter[SE_IA]->getValue();
    const double raa = parameter[SE_RA]->getValue();
    const double headroom = parameter[SE_HEADROOM]->getValue();

    if (!device1 || vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        return;
    }

    const int steps = 32;
    operatingPoints.reserve(steps);
    harmonicMatrix.resize(4); // HD2, HD3, HD4, HD5
    for (int i = 0; i < 4; ++i) {
        harmonicMatrix[i].reserve(steps);
    }

    if (sweepBias) {
        // Sweep bias current (Ia) at fixed headroom
        const double iaMin = std::max(ia * 0.5, 1.0);
        const double iaMax = std::min(ia * 1.5, device1->getIaMax());
        
        for (int i = 0; i <= steps; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(steps);
            const double iaBias = iaMin + (iaMax - iaMin) * t;
            
            double hd2 = 0.0, hd3 = 0.0, hd4 = 0.0, hd5 = 0.0, thd = 0.0;
            if (simulateHarmonicsTimeDomain(vb, iaBias, raa, headroom, vs, hd2, hd3, hd4, hd5, thd)) {
                operatingPoints.push_back(iaBias);
                harmonicMatrix[0].push_back(hd2);
                harmonicMatrix[1].push_back(hd3);
                harmonicMatrix[2].push_back(hd4);
                harmonicMatrix[3].push_back(hd5);
            }
        }
    } else {
        // Sweep headroom at fixed bias
        const double maxHeadroom = 0.9 * vb;
        
        for (int i = 1; i <= steps; ++i) {
            const double head = maxHeadroom * static_cast<double>(i) / static_cast<double>(steps);
            
            double hd2 = 0.0, hd3 = 0.0, hd4 = 0.0, hd5 = 0.0, thd = 0.0;
            if (simulateHarmonicsTimeDomain(vb, ia, raa, head, vs, hd2, hd3, hd4, hd5, thd)) {
                operatingPoints.push_back(head);
                harmonicMatrix[0].push_back(hd2);
                harmonicMatrix[1].push_back(hd3);
                harmonicMatrix[2].push_back(hd4);
                harmonicMatrix[3].push_back(hd5);
            }
        }
    }
}

void SingleEndedOutput::computeHarmonicSurfaceData(QVector<double> &biasPoints,
                                                   QVector<double> &headroomPoints,
                                                   QVector<QVector<QVector<double>>> &harmonicSurface) const
{
    biasPoints.clear();
    headroomPoints.clear();
    harmonicSurface.clear();

    const double vb = parameter[SE_VB]->getValue();
    const double vs = parameter[SE_VS]->getValue();
    const double iaCenter = parameter[SE_IA]->getValue();
    const double raa = parameter[SE_RA]->getValue();

    if (!device1 || vb <= 0.0 || raa <= 0.0 || iaCenter <= 0.0) {
        return;
    }

    // Generate grid of bias and headroom points
    const int biasSteps = 24;
    const int headroomSteps = 16;

    // Bias current range
    const double iaMin = std::max(iaCenter * 0.6, 1.0);
    const double iaMax = std::min(iaCenter * 1.4, device1->getIaMax());

    // Headroom range
    const double maxHeadroom = 0.9 * vb;
    const double headroomMin = maxHeadroom * 0.2;
    const double headroomMax = maxHeadroom * 0.9;

    biasPoints.reserve(biasSteps);
    headroomPoints.reserve(headroomSteps);
    harmonicSurface.resize(4); // HD2, HD3, HD4, HD5

    for (int i = 0; i < 4; ++i) {
        harmonicSurface[i].resize(headroomSteps);
        for (int j = 0; j < headroomSteps; ++j) {
            harmonicSurface[i][j].resize(biasSteps);
        }
    }

    // Generate bias points
    for (int i = 0; i < biasSteps; ++i) {
        double bias = iaMin + (iaMax - iaMin) * i / (biasSteps - 1);
        biasPoints.push_back(bias);
    }

    // Generate headroom points
    for (int i = 0; i < headroomSteps; ++i) {
        double headroom = headroomMin + (headroomMax - headroomMin) * i / (headroomSteps - 1);
        headroomPoints.push_back(headroom);
    }

    // Compute harmonic data for each grid point
    for (int hIdx = 0; hIdx < headroomSteps; ++hIdx) {
        double headroom = headroomPoints[hIdx];
        
        for (int bIdx = 0; bIdx < biasSteps; ++bIdx) {
            double bias = biasPoints[bIdx];
            
            double hd2 = 0.0, hd3 = 0.0, hd4 = 0.0, hd5 = 0.0, thd = 0.0;
            // FIX: Use headroom as signal amplitude instead of vs parameter for realistic clipping
            if (simulateHarmonicsTimeDomain(vb, bias, raa, headroom, headroom, hd2, hd3, hd4, hd5, thd)) {
                harmonicSurface[0][hIdx][bIdx] = hd2; // HD2
                harmonicSurface[1][hIdx][bIdx] = hd3; // HD3
                harmonicSurface[2][hIdx][bIdx] = hd4; // HD4
                harmonicSurface[3][hIdx][bIdx] = hd5; // HD5
            } else {
                harmonicSurface[0][hIdx][bIdx] = 0.0;
                harmonicSurface[1][hIdx][bIdx] = 0.0;
                harmonicSurface[2][hIdx][bIdx] = 0.0;
                harmonicSurface[3][hIdx][bIdx] = 0.0;
            }
        }
    }
}

QPointF SingleEndedOutput::findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
                                                const QPointF &line2Start, const QPointF &line2End) const
{
    const double x1 = line1Start.x(), y1 = line1Start.y();
    const double x2 = line1End.x(),   y2 = line1End.y();
    const double x3 = line2Start.x(), y3 = line2Start.y();
    const double x4 = line2End.x(),   y4 = line2End.y();

    const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(denom) < 1e-12) {
        return QPointF(-1.0, -1.0);
    }

    const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0) {
        return QPointF(-1.0, -1.0);
    }

    const double ix = x1 + t * (x2 - x1);
    const double iy = y1 + t * (y2 - y1);
    return QPointF(ix, iy);
}

void SingleEndedOutput::update(int index)
{
    Q_UNUSED(index);

    if (!device1) {
        parameter[SE_VK]->setValue(0.0);
        parameter[SE_IK]->setValue(0.0);
        parameter[SE_RK]->setValue(0.0);
        parameter[SE_POUT]->setValue(0.0);
        parameter[SE_PHEAD]->setValue(0.0);
        parameter[SE_HD2]->setValue(0.0);
        parameter[SE_HD3]->setValue(0.0);
        parameter[SE_HD4]->setValue(0.0);
        parameter[SE_THD]->setValue(0.0);
        return;
    }

    const double vb       = parameter[SE_VB]->getValue();
    const double vs       = parameter[SE_VS]->getValue();
    const double ia       = parameter[SE_IA]->getValue(); // mA
    const double raa      = parameter[SE_RA]->getValue();
    const double headroom = parameter[SE_HEADROOM]->getValue(); // Vpk at anode (manual input)

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        parameter[SE_VK]->setValue(0.0);
        parameter[SE_IK]->setValue(0.0);
        parameter[SE_RK]->setValue(0.0);
        parameter[SE_POUT]->setValue(0.0);
        parameter[SE_PHEAD]->setValue(0.0);
        parameter[SE_HD2]->setValue(0.0);
        parameter[SE_HD3]->setValue(0.0);
        parameter[SE_HD4]->setValue(0.0);
        parameter[SE_THD]->setValue(0.0);
        return;
    }
    const double vg1Max = device1->getVg1Max();
    const double vaMax  = device1->getVaMax();

    // Class B load line (not currently plotted, but kept for parity)
    const double iaMaxB = 4000.0 * vb / raa;
    QVector<QPointF> classBLine;
    classBLine.push_back(QPointF(0.0, iaMaxB));
    classBLine.push_back(QPointF(vb, 0.0));

    // AC load line around the chosen bias current ia
    const double gradient = -1000.0 / raa;          // mA/V
    const double iaMaxA   = ia - gradient * vb;     // Ia at Va = 0
    const double vaMaxA   = -iaMaxA / gradient;     // Va intercept

    QVector<QPointF> acLine;
    for (int i = 0; i < 101; ++i) {
        const double va = static_cast<double>(i) * vaMaxA / 100.0;
        const double ia2 = iaMaxA - va * 1000.0 / raa;
        acLine.push_back(QPointF(va, ia2));
    }

    // Anode characteristics at Vg1=0, Vg2=Vs
    QVector<QPointF> anodeCurve0;
    for (int i = 1; i < 101; ++i) {
        const double va = vaMax * static_cast<double>(i) / 100.0;
        double ia0_mA = device1->anodeCurrent(va, 0.0, vs) * 1000.0;
        if (std::isfinite(ia0_mA) && ia0_mA >= 0.0) {
            anodeCurve0.push_back(QPointF(va, ia0_mA));
        }
    }

    double pout_W = 0.0;
    if (anodeCurve0.size() >= 2) {
        const QPointF aStart = acLine.front();
        const QPointF aEnd   = acLine.back();

        QPointF best(-1.0, -1.0);
        for (int i = 0; i < anodeCurve0.size() - 1; ++i) {
            const QPointF cStart = anodeCurve0[i];
            const QPointF cEnd   = anodeCurve0[i + 1];
            const QPointF ip     = findLineIntersection(aStart, aEnd, cStart, cEnd);
            if (ip.x() >= 0.0 && ip.y() >= 0.0) {
                best = ip;
                break;
            }
        }

        if (best.x() >= 0.0 && best.y() >= 0.0) {
            pout_W = (vb - best.x()) * best.y() / 2000.0;
        }
    }

    // Prefer using embedded measurement data (if present on this Device) to
    // determine Vk and Ig2 at the requested bias. This makes the SE numeric
    // panel match the analyser's measured idle more closely. Fall back to the
    // fitted model surface when no suitable measurement data is available.
    double bestVg1 = 0.0;   // Vk (positive magnitude of grid bias)
    double ik_mA = ia;
    double ig2_mA = 0.0;

    bool usedMeasurementBias = false;
    if (device1 && device1->getMeasurement()) {
        double vk_meas = 0.0;
        double ig2_meas_mA = 0.0;
        if (device1->findBiasFromMeasurement(vb, vs, ia, vk_meas, ig2_meas_mA)) {
            bestVg1 = vk_meas;
            ig2_mA = ig2_meas_mA;
            ik_mA = ia + ig2_mA;
            usedMeasurementBias = true;
        }
    }

    if (!usedMeasurementBias) {
        // Existing model-based path: find Vk (grid bias) such that
        // Ia(Vb, -Vk, Vs) ~= ia using the fitted model.
        double minErr = std::numeric_limits<double>::infinity();
        const int vgSteps = 1000;
        for (int i = 0; i <= vgSteps; ++i) {
            const double vg1 = vg1Max * static_cast<double>(i) / vgSteps;
            double ia_test_mA = device1->anodeCurrent(vb, -vg1, vs) * 1000.0;
            if (!std::isfinite(ia_test_mA)) continue;
            const double err = std::abs(ia - ia_test_mA);
            if (err < minErr) {
                minErr = err;
                bestVg1 = vg1;
            }
        }

        ik_mA = ia;
        ig2_mA = 0.0;
        if (device1->getDeviceType() == PENTODE) {
            // Device::screenCurrent already returns mA (see Model::screenCurrent
            // convention), so add it directly without additional scaling.
            ig2_mA = device1->screenCurrent(vb, -bestVg1, vs);
            ik_mA += ig2_mA;
        }
    }

    double rk_ohms = 0.0;
    if (ik_mA > 0.0) {
        rk_ohms = 1000.0 * bestVg1 / ik_mA;
    }

    // Diagnostics: log SE operating point and model currents so we can compare
    // against analyser expectations (e.g., Ig2 ~1.5 mA at typical 6L6 bias).
    qInfo("SE_OUTPUT OP: Vb=%.3f Vs=%.3f Vk=%.3f (Vg1=-Vk=%.3f) Ia=%.3f mA Ig2=%.3f mA Ik=%.3f mA Rk=%.3f ohms",
          vb, vs, bestVg1, -bestVg1, ia, ig2_mA, ik_mA, rk_ohms);

    parameter[SE_VK]->setValue(bestVg1);
    parameter[SE_IK]->setValue(ik_mA);
    parameter[SE_RK]->setValue(rk_ohms);
    parameter[SE_POUT]->setValue(pout_W);

    // Determine effective headroom (Vpk at anode) driving PHEAD/THD/sensitivity.
    // - If manual Headroom>0, use that directly.
    // - If Headroom==0 and swing helpers are available, use Vpp_sym/2 when
    //   showSymSwing is true, otherwise use Vpp_max/2.
    double symVpp = 0.0;
    double maxVpp = 0.0;

    // Recompute swing geometry in data space (no drawing) using same logic
    // as the plot() swing helper.
    {
        const double slope = -1000.0 / raa; // mA/V, same as AC load line
        const double ia0   = ia;
        const double va0   = vb;

        auto ia_line_mA = [&](double va_val) {
            return ia0 + slope * (va_val - va0);
        };

        // Left limit: intersection of AC load line with Vg1 = 0 curve at current screen voltage
        double vaLeft = -1.0;
        auto f_left = [&](double va_val) {
            double ia_curve_mA = device1->anodeCurrent(va_val, 0.0, vs) * 1000.0;
            double ia_line = ia_line_mA(va_val);
            return ia_curve_mA - ia_line;
        };

        {
            const int samples = 400;
            double lastVa = std::clamp(va0, 0.0, vaMax);
            double lastF  = f_left(lastVa);
            for (int i = 1; i <= samples; ++i) {
                double va_val = va0 * (1.0 - static_cast<double>(i) / samples);
                va_val = std::max(va_val, 0.0);
                double curF = f_left(va_val);
                if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                    double denom = (curF - lastF);
                    double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                    t = std::clamp(t, 0.0, 1.0);
                    vaLeft = lastVa + t * (va_val - lastVa);
                    break;
                }
                lastVa = va_val;
                lastF  = curF;
            }
        }

        // Right limits: Ia = 0 and Pa_max
        double vaRight = -1.0;
        if (vaLeft >= 0.0) {
            // Ia = 0 crossing
            double vaZero = va0 - ia0 / slope;
            vaZero = std::clamp(vaZero, 0.0, vaMax);

            double vaPa = vaMax + 1.0;
            const double paMaxW = device1->getPaMax();
            if (paMaxW > 0.0) {
                auto g_pa = [&](double va_val) {
                    if (va_val <= 0.0) return 1e9;
                    double ia_line = ia_line_mA(va_val);
                    double ia_pa_mA = 1000.0 * paMaxW / va_val;
                    return ia_line - ia_pa_mA;
                };
                const int samples = 400;
                double lastVa = std::max(va0, 1e-3);
                double lastF  = g_pa(lastVa);
                for (int i = 1; i <= samples; ++i) {
                    double va_val = va0 + (vaMax - va0) * (static_cast<double>(i) / samples);
                    double curF = g_pa(va_val);
                    if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                        double denom = (curF - lastF);
                        double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                        t = std::clamp(t, 0.0, 1.0);
                        vaPa = lastVa + t * (va_val - lastVa);
                        break;
                    }
                    lastVa = va_val;
                    lastF  = curF;
                }
            }

            vaRight = std::min(vaZero, vaPa);
            vaRight = std::clamp(vaRight, 0.0, vaMax);
        }

        if (vaLeft >= 0.0 && vaRight > va0 && vaLeft < va0) {
            maxVpp = vaRight - vaLeft;

            const double vpk_sym = std::min(va0 - vaLeft, vaRight - va0);
            if (vpk_sym > 0.0) {
                symVpp = 2.0 * vpk_sym;
            }
        }
    }

    // Apply selection rules for effectiveHeadroomVpk.
    double effective = 0.0;
    if (headroom > 0.0) {
        effective = headroom;
    } else {
        if (showSymSwing && symVpp > 0.0) {
            effective = symVpp / 2.0;
        } else if (maxVpp > 0.0) {
            effective = maxVpp / 2.0;
        }
    }
    effectiveHeadroomVpk = effective;

    // Simple power-at-headroom helper using linear load assumption around OP.
    double phead_W = 0.0;
    if (effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        // Treat effectiveHeadroomVpk as anode peak swing; output power is Vpk^2 / (2 * Ra).
        phead_W = (effectiveHeadroomVpk * effectiveHeadroomVpk) / (2.0 * raa);
    }
    parameter[SE_PHEAD]->setValue(phead_W);

    double hd2 = 0.0;
    double hd3 = 0.0;
    double hd4 = 0.0;
    double thd = 0.0;

    parameter[SE_HD2]->setValue(0.0);
    parameter[SE_HD3]->setValue(0.0);
    parameter[SE_HD4]->setValue(0.0);
    parameter[SE_THD]->setValue(0.0);

    // THD should be computed whenever we have a positive effective headroom,
    // regardless of whether it came from the manual Headroom parameter or from
    // the max/sym swing helpers.
    if (effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        double Ia = 0.0, Ib = 0.0, Ic = 0.0, Id = 0.0, Ie = 0.0;
        if (computeHeadroomHarmonicCurrents(vb, ia, raa, effectiveHeadroomVpk, vs,
                                            Ia, Ib, Ic, Id, Ie)) {
            computeHarmonics(Ia, Ib, Ic, Id, Ie, hd2, hd3, hd4, thd);

            // If cathode is unbypassed (gainMode == 0), approximate the effect of
            // local feedback by reducing the harmonic amplitudes by a simple
            // (1 + gm*Rk) factor derived from the current bias point.
            if (gainMode == 0 && rk_ohms > 0.0) {
                // Reuse the same small-signal gm approximation used for
                // sensitivity, around Vgk = -Vk at the present bias.
                const double vk      = bestVg1;
                const double vgBias  = -vk;
                const double dVg     = std::max(0.05, std::abs(vgBias) * 0.02);
                double iaPlus_A      = device1->anodeCurrent(vb, vgBias + dVg, vs);
                double iaMinus_A     = device1->anodeCurrent(vb, vgBias - dVg, vs);
                double gm_mA_per_V   = 0.0;
                if (std::isfinite(iaPlus_A) && std::isfinite(iaMinus_A) && dVg > 0.0) {
                    gm_mA_per_V = (iaPlus_A - iaMinus_A) * 1000.0 / (2.0 * dVg);
                }
                if (std::isfinite(gm_mA_per_V)) {
                    const double gm_A_per_V = gm_mA_per_V / 1000.0;
                    const double feedback   = 1.0 + gm_A_per_V * rk_ohms;
                    if (feedback > 1.0 && std::isfinite(feedback)) {
                        hd2 /= feedback;
                        hd3 /= feedback;
                        hd4 /= feedback;
                        thd /= feedback;
                    }
                }
            }

            parameter[SE_HD2]->setValue(hd2);
            parameter[SE_HD3]->setValue(hd3);
            parameter[SE_HD4]->setValue(hd4);
            parameter[SE_THD]->setValue(thd);
        }
    }
}

double SingleEndedOutput::dcLoadlineCurrent(double vb, double raa, double va) const
{
    const double q = vb / raa;
    const double m = -q / vb;
    return m * va + q;
}

double SingleEndedOutput::findGridBiasForCurrent(double targetIa_A,
                                                 double vb,
                                                 double vs,
                                                 double raa) const
{
    const double va = vb - targetIa_A * raa;
    if (va <= 0.0 || !std::isfinite(va)) {
        return 0.0;
    }

    const double vg1Max = device1->getVg1Max();
    double bestVg1 = 0.0;
    double minErr = std::numeric_limits<double>::infinity();
    const int vgSteps = 400;

    for (int i = 0; i <= vgSteps; ++i) {
        const double vg1 = vg1Max * static_cast<double>(i) / vgSteps;
        const double iaTest = device1->anodeCurrent(va, -vg1, vs);
        if (!std::isfinite(iaTest) || iaTest < 0.0) {
            continue;
        }
        const double err = std::abs(targetIa_A - iaTest);
        if (err < minErr) {
            minErr = err;
            bestVg1 = vg1;
        }
    }

    return bestVg1;
}

double SingleEndedOutput::findVaFromVg(double vg1,
                                       double vb,
                                       double vs,
                                       double raa) const
{
    double va = 0.0;
    double incr = vb / 10.0;

    for (;;) {
        const double it = device1->anodeCurrent(va, -vg1, vs);
        const double il = dcLoadlineCurrent(vb, raa, va);

        if (!std::isfinite(it) || !std::isfinite(il)) {
            break;
        }

        if (it >= il && incr <= 1e-6) {
            break;
        } else if (it >= il) {
            va -= incr;
            incr *= 0.1;
        }

        va += incr;

        if (va < 0.0 || va > 2.0 * vb) {
            break;
        }
    }

    return va;
}

bool SingleEndedOutput::computeHeadroomHarmonicCurrents(double vb,
                                                        double ia_mA,
                                                        double raa,
                                                        double headroom,
                                                        double vs,
                                                        double &Ia,
                                                        double &Ib,
                                                        double &Ic,
                                                        double &Id,
                                                        double &Ie) const
{
    const double biasCurrent_A = ia_mA / 1000.0;

    const double qA = vb / raa;
    const double mA = -qA / vb;
    const double vOperating = (biasCurrent_A - qA) / mA;

    double vMin = vOperating - headroom;
    double vMax = vOperating + headroom;

    // For very large headroom values, the ideal sinusoid may demand swings
    // past Va=0 or beyond the plotted range. Instead of bailing out, clamp
    // to a small positive minimum and a reasonable upper bound so we can
    // continue to estimate THD into clipping.
    const double kMinVa = 1e-3;
    const double kMaxVa = 2.0 * vb;
    vMin = std::max(vMin, kMinVa);
    vMax = std::clamp(vMax, vMin + 1e-6, kMaxVa);

    const double I_max = dcLoadlineCurrent(vb, raa, vMin);
    const double I_min = dcLoadlineCurrent(vb, raa, vMax);
    if (!std::isfinite(I_max) || !std::isfinite(I_min)) {
        return false;
    }

    const double Vg_bias = findGridBiasForCurrent(biasCurrent_A, vb, vs, raa);
    const double Vg_max  = findGridBiasForCurrent(I_max,          vb, vs, raa);

    const double Vg_max_mid = Vg_bias + (Vg_max - Vg_bias) / 2.0;
    const double Vg_min_mid = Vg_bias - (Vg_max - Vg_bias) / 2.0;
    const double Vg_min     = Vg_bias - (Vg_max - Vg_bias);

    const double V_min_mid_distorted = findVaFromVg(Vg_max_mid, vb, vs, raa);
    const double V_max_mid_distorted = findVaFromVg(Vg_min_mid, vb, vs, raa);
    const double V_max_distorted     = findVaFromVg(Vg_min,     vb, vs, raa);

    const double I_max_mid_distorted = dcLoadlineCurrent(vb, raa, V_min_mid_distorted);
    const double I_min_mid_distorted = dcLoadlineCurrent(vb, raa, V_max_mid_distorted);
    const double I_min_distorted     = dcLoadlineCurrent(vb, raa, V_max_distorted);

    if (!std::isfinite(I_max_mid_distorted) ||
        !std::isfinite(I_min_mid_distorted) ||
        !std::isfinite(I_min_distorted)) {
        return false;
    }

    Ia = I_max;
    Ib = I_max_mid_distorted;
    Ic = biasCurrent_A;
    Id = I_min_mid_distorted;
    Ie = I_min_distorted;

    return true;
}

bool SingleEndedOutput::computeHeadroomPolygonPoints(double vb,
                                                     double ia_mA,
                                                     double raa,
                                                     double headroom,
                                                     double vs,
                                                     double &vaMin,
                                                     double &iaMax,
                                                     double &vaMaxDistorted,
                                                     double &iaMinDistorted) const
{
    const double biasCurrent_A = ia_mA / 1000.0;

    const double qA = vb / raa;
    const double mA = -qA / vb;
    const double vOperating = (biasCurrent_A - qA) / mA;

    double vMin = vOperating - headroom;
    double vMax = vOperating + headroom;

    // Allow large headroom up to clipping by clamping Va range instead of
    // rejecting swings that would go negative or far beyond the plotted
    // domain.
    const double kMinVa = 1e-3;
    const double kMaxVa = 2.0 * vb;
    vMin = std::max(vMin, kMinVa);
    vMax = std::clamp(vMax, vMin + 1e-6, kMaxVa);

    const double I_max = dcLoadlineCurrent(vb, raa, vMin);      // amps
    if (!std::isfinite(I_max)) {
        return false;
    }

    const double Vg_bias = findGridBiasForCurrent(biasCurrent_A, vb, vs, raa);
    const double Vg_max  = findGridBiasForCurrent(I_max,          vb, vs, raa);

    const double Vg_min  = Vg_bias - (Vg_max - Vg_bias);
    const double V_max_distorted = findVaFromVg(Vg_min, vb, vs, raa);
    const double I_min_distorted = dcLoadlineCurrent(vb, raa, V_max_distorted); // amps

    if (!std::isfinite(I_min_distorted)) {
        return false;
    }

    vaMin          = vMin;
    vaMaxDistorted = V_max_distorted;

    // Convert currents to mA for plotting on the Ia axis
    iaMax          = I_max * 1000.0;
    iaMinDistorted = I_min_distorted * 1000.0;

    return true;
}

void SingleEndedOutput::computeHarmonics(double Ia,
                                         double Ib,
                                         double Ic,
                                         double Id,
                                         double Ie,
                                         double &hd2,
                                         double &hd3,
                                         double &hd4,
                                         double &thd) const
{
    const double denom = Ia + Ib - Id - Ie;
    if (std::abs(denom) < 1e-12) {
        hd2 = hd3 = hd4 = thd = 0.0;
        return;
    }

    hd2 = std::abs(75.0 * (Ia + Ie - 2.0 * Ic) / denom);
    hd3 = std::abs(50.0 * (Ia - 2.0 * Ib + 2.0 * Id - Ie) / denom);
    hd4 = std::abs(25.0 * (Ia - 4.0 * Ib + 6.0 * Ic - 4.0 * Id + Ie) / denom);
    thd = std::sqrt(hd2 * hd2 + hd3 * hd3 + hd4 * hd4);
}

bool SingleEndedOutput::simulateHarmonicsTimeDomain(double vb,
                                                    double iaBias_mA,
                                                    double raa,
                                                    double headroomVpk,
                                                    double vs,
                                                    double &hd2,
                                                    double &hd3,
                                                    double &hd4,
                                                    double &hd5,
                                                    double &thd) const
{
    // Initialise outputs to a safe default.
    hd2 = 0.0;
    hd3 = 0.0;
    hd4 = 0.0;
    hd5 = 0.0;
    thd = 0.0;

    if (!device1) {
        return false;
    }

    // Basic sanity checks: require a valid operating point and positive headroom.
    if (vb <= 0.0 || raa <= 0.0 || iaBias_mA <= 0.0 || headroomVpk <= 0.0) {
        return false;
    }

    // Reuse the existing VTADIY-style 5-point helper to obtain the base current
    // samples along the AC load line. These are expressed as currents on the
    // DC load line at five effective swing positions.
    double Ia = 0.0;  // I_max
    double Ib = 0.0;  // I_max_mid_distorted
    double Ic = 0.0;  // I_bias
    double Id = 0.0;  // I_min_mid_distorted
    double Ie = 0.0;  // I_min_distorted
    if (!computeHeadroomHarmonicCurrents(vb,
                                         iaBias_mA,
                                         raa,
                                         headroomVpk,
                                         vs,
                                         Ia,
                                         Ib,
                                         Ic,
                                         Id,
                                         Ie)) {
        return false;
    }

    // Arrange the five samples (in amps) as one period of a periodic waveform.
    // The samples are treated as equally spaced over the cycle and interpolated
    // linearly for higher-resolution time-domain sampling.
    double samples[5];
    samples[0] = Ia;
    samples[1] = Ib;
    samples[2] = Ic;
    samples[3] = Id;
    samples[4] = Ie;

    // Use a modest number of samples to keep this helper inexpensive while
    // providing a smooth waveform for the DFT. A Hann window is applied to
    // reduce spectral leakage, following the style used in Cavern QuickEQ.
    const int sampleCount = 512;
    const double twoPi = 6.28318530717958647692; // 2 * pi

    double a[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double b[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    for (int k = 0; k < sampleCount; ++k) {
        const double phase = twoPi * static_cast<double>(k) / static_cast<double>(sampleCount);
        const double u = phase / twoPi; // normalised phase in [0, 1)

        // Map the normalised phase to the five base samples, then linearly
        // interpolate between neighbouring samples to obtain a dense waveform.
        const double pos = u * 5.0; // 5 samples per period
        const double indexF = std::floor(pos);
        int i0 = static_cast<int>(indexF);
        if (i0 < 0) {
            i0 = 0;
        }
        if (i0 >= 5) {
            i0 = i0 % 5;
        }
        const int i1 = (i0 + 1) % 5;
        const double frac = pos - indexF;

        const double ip = samples[i0] + (samples[i1] - samples[i0]) * frac;

        // Hann window across the full set of samples.
        const double window = 0.5 * (1.0 - std::cos(twoPi * static_cast<double>(k) /
                                                   static_cast<double>(sampleCount - 1)));
        const double v = ip * window;

        // Accumulate the first five harmonic components via a small manual DFT.
        for (int n = 1; n <= 5; ++n) {
            const double angle = static_cast<double>(n) * phase;
            const double c = std::cos(angle);
            const double s = std::sin(angle);
            a[n] += v * c;
            b[n] += v * s;
        }
    }

    const double scale = 2.0 / static_cast<double>(sampleCount);
    double A[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    for (int n = 1; n <= 5; ++n) {
        a[n] *= scale;
        b[n] *= scale;
        A[n] = std::sqrt(a[n] * a[n] + b[n] * b[n]);
    }

    const double fundamental = A[1];
    if (!(fundamental > 0.0) || !std::isfinite(fundamental)) {
        return false;
    }

    // Express harmonic levels as percentages relative to the fundamental,
    // mirroring the interpretation used by the existing 5-point helper.
    const double invFund = 100.0 / fundamental;
    hd2 = A[2] * invFund;
    hd3 = A[3] * invFund;
    hd4 = A[4] * invFund;
    hd5 = A[5] * invFund;

    if (!std::isfinite(hd2) || hd2 < 0.0) hd2 = 0.0;
    if (!std::isfinite(hd3) || hd3 < 0.0) hd3 = 0.0;
    if (!std::isfinite(hd4) || hd4 < 0.0) hd4 = 0.0;
    if (!std::isfinite(hd5) || hd5 < 0.0) hd5 = 0.0;

    thd = std::sqrt(hd2 * hd2 + hd3 * hd3 + hd4 * hd4 + hd5 * hd5);
    if (!std::isfinite(thd) || thd < 0.0) {
        thd = 0.0;
    }

    return true;
}

void SingleEndedOutput::computeTimeDomainHarmonicScan(QVector<double> &headroomVals,
                                                      QVector<double> &hd2Vals,
                                                      QVector<double> &hd3Vals,
                                                      QVector<double> &hd4Vals,
                                                      QVector<double> &thdVals) const
{
    headroomVals.clear();
    hd2Vals.clear();
    hd3Vals.clear();
    hd4Vals.clear();
    thdVals.clear();

    const double vb  = parameter[SE_VB]->getValue();
    const double vs  = parameter[SE_VS]->getValue();
    const double ia  = parameter[SE_IA]->getValue();
    const double raa = parameter[SE_RA]->getValue();

    if (!device1 || vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        return;
    }

    const double maxHeadroom = 0.9 * vb;
    const int steps = 32;
    headroomVals.reserve(steps);
    hd2Vals.reserve(steps);
    hd3Vals.reserve(steps);
    hd4Vals.reserve(steps);
    thdVals.reserve(steps);

    for (int i = 1; i <= steps; ++i) {
        const double head = maxHeadroom * static_cast<double>(i) / static_cast<double>(steps);
        double hd2 = 0.0;
        double hd3 = 0.0;
        double hd4 = 0.0;
        double hd5 = 0.0;
        double thd = 0.0;
        if (simulateHarmonicsTimeDomain(vb, ia, raa, head, vs, hd2, hd3, hd4, hd5, thd)) {
            headroomVals.push_back(head);
            hd2Vals.push_back(hd2);
            hd3Vals.push_back(hd3);
            hd4Vals.push_back(hd4);
            thdVals.push_back(thd);
        }
    }
}

void SingleEndedOutput::debugScanHeadroomTimeDomain() const
{
    QVector<double> headroomVals;
    QVector<double> hd2Vals;
    QVector<double> hd3Vals;
    QVector<double> hd4Vals;
    QVector<double> thdVals;

    computeTimeDomainHarmonicScan(headroomVals, hd2Vals, hd3Vals, hd4Vals, thdVals);

    const int count = headroomVals.size();
    for (int i = 0; i < count; ++i) {
        const double head = headroomVals[i];
        const double hd2  = (i < hd2Vals.size()) ? hd2Vals[i] : 0.0;
        const double hd3  = (i < hd3Vals.size()) ? hd3Vals[i] : 0.0;
        const double hd4  = (i < hd4Vals.size()) ? hd4Vals[i] : 0.0;
        const double thd  = (i < thdVals.size()) ? thdVals[i] : 0.0;
        qInfo("SE_THD_SCAN: headroom=%.3f hd2=%.3f hd3=%.3f hd4=%.3f thd=%.3f",
              head, hd2, hd3, hd4, thd);
    }
}

void SingleEndedOutput::plot(Plot *plot)
{
    if (!device1) {
        return;
    }

    // Clear previous overlays
    if (anodeLoadLine) {
        plot->getScene()->removeItem(anodeLoadLine);
        delete anodeLoadLine;
        anodeLoadLine = nullptr;
    }
    if (cathodeLoadLine) {
        plot->getScene()->removeItem(cathodeLoadLine);
        delete cathodeLoadLine;
        cathodeLoadLine = nullptr;
    }
    if (acSignalLine) {
        plot->getScene()->removeItem(acSignalLine);
        delete acSignalLine;
        acSignalLine = nullptr;
    }
    if (opMarker) {
        plot->getScene()->removeItem(opMarker);
        delete opMarker;
        opMarker = nullptr;
    }

    const double vaMax = device1->getVaMax();
    const double iaMax = device1->getIaMax();
    const double xMajor = std::max(5.0, vaMax / 10.0);
    const double yMajor = std::max(0.5, iaMax / 10.0);

    // Only set axes when the scene is empty so that re-plotting (for example
    // when toggling Max Sym Swing) does not clear existing model or
    // measurement overlays that the Designer shares with Modeller/Analyser.
    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, vaMax, xMajor, 0.0, iaMax, yMajor);
    }

    const double vb  = parameter[SE_VB]->getValue();
    const double vs  = parameter[SE_VS]->getValue();
    const double ia       = parameter[SE_IA]->getValue();
    const double raa      = parameter[SE_RA]->getValue();
    const double headroom = parameter[SE_HEADROOM]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        return;
    }

    // Recreate AC load line for plotting (through the chosen operating point)
    const double gradient = -1000.0 / raa;            // mA/V
    const double iaMaxA   = ia - gradient * vb;       // Ia at Va = 0
    const double vaMaxA   = -iaMaxA / gradient;       // Va intercept

    QVector<QPointF> acLine;
    acLine.reserve(101);
    for (int i = 0; i < 101; ++i) {
        const double va = static_cast<double>(i) * vaMaxA / 100.0;
        const double ia2 = iaMaxA - va * 1000.0 / raa;
        acLine.push_back(QPointF(va, ia2));
    }

    // DC load line from (0, Vb/Ra) to (Vb, 0) for reference (green, like Triode CC)
    if (raa > 0.0) {
        cathodeLoadLine = new QGraphicsItemGroup();
        QPen dcPen;
        dcPen.setColor(QColor::fromRgb(0, 128, 0));
        dcPen.setWidth(2);

        const double ia_dc_0 = (vb / raa) * 1000.0; // mA at Va=0
        const double x1 = 0.0;
        const double y1 = std::clamp(ia_dc_0, 0.0, iaMax);
        const double x2 = std::min(vb, vaMax);
        const double y2 = 0.0;

        if (auto *seg = plot->createSegment(x1, y1, x2, y2, dcPen)) {
            cathodeLoadLine->addToGroup(seg);
        }

        if (!cathodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(cathodeLoadLine);
        } else {
            delete cathodeLoadLine;
            cathodeLoadLine = nullptr;
        }
    }

    // Draw AC load line (yellow), matching Triode CC small-signal line colour,
    // and overlay the Pa_max (anode dissipation) limit as a dashed pink line.
    acSignalLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(255, 215, 0));
        pen.setWidth(2);
        for (int i = 0; i < acLine.size() - 1; ++i) {
            const QPointF s = acLine[i];
            const QPointF e = acLine[i + 1];
            if (auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen)) {
                acSignalLine->addToGroup(seg);
            }
        }

        // Pa_max hyperbola: Ia = Pa_max * 1000 / Va, clipped to the visible Ia range.
        const double paMaxW = device1->getPaMax();
        if (paMaxW > 0.0) {
            QPen paPen(QColor::fromRgb(255, 105, 180)); // pink, dashed
            paPen.setStyle(Qt::DashLine);
            paPen.setWidth(2);

            const double xStop = vaMax;
            const double yStop = iaMax;
            const double xEnter = std::max(1e-6, std::min(xStop, (yStop > 0.0 ? (1000.0 * paMaxW / yStop) : xStop)));

            const int segs = 60;
            double prevX = xEnter;
            double prevY = std::min(yStop, 1000.0 * paMaxW / prevX);
            for (int i = 1; i <= segs; ++i) {
                double t = static_cast<double>(i) / segs;
                double x = xEnter + (xStop - xEnter) * t;
                double y = (x > 0.0) ? std::min(yStop, 1000.0 * paMaxW / x) : yStop;
                if (auto *seg = plot->createSegment(prevX, prevY, x, y, paPen)) {
                    acSignalLine->addToGroup(seg);
                }
                prevX = x;
                prevY = y;
            }
        }
        if (!acSignalLine->childItems().isEmpty()) {
            plot->getScene()->addItem(acSignalLine);
        } else {
            delete acSignalLine;
            acSignalLine = nullptr;
        }
    }

    // Mark the DC operating point at (Vb, Ia) on the load line
    opMarker = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(255, 0, 0));
        pen.setWidth(2);
        const double d = 5.0;
        if (auto *h = plot->createSegment(vb - d, ia, vb + d, ia, pen)) opMarker->addToGroup(h);
        if (auto *v = plot->createSegment(vb, ia - d, vb, ia + d, pen)) opMarker->addToGroup(v);

        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker;
            opMarker = nullptr;
        }
    }

    // Draw a simple headroom segment around the operating point along the AC load line,
    // in the original VTlady style, with a filled polygon down to Ia = 0.
    if (headroom > 0.0) {
        double vaMin = vb - headroom;
        double vaMax2 = vb + headroom;

        // Clamp to the plotted Va range
        vaMin  = std::max(0.0, vaMin);
        vaMax2 = std::min(vaMax, vaMax2);

        if (vaMax2 > vaMin) {
            const double dia = headroom * 1000.0 / raa; // mA deviation along load line

            anodeLoadLine = new QGraphicsItemGroup();
            QPen pen;
            pen.setColor(QColor::fromRgb(0, 0, 255));
            pen.setWidth(2);

            const double iaHigh = ia + dia;
            const double iaLow  = ia - dia;

            // Blue headroom line along the AC load line
            if (auto *seg = plot->createSegment(vaMin, iaHigh, vaMax2, iaLow, pen)) {
                anodeLoadLine->addToGroup(seg);
            }

            // Filled polygon in scene coordinates under the headroom line
            const double xScale = PLOT_WIDTH  / (vaMax - 0.0);
            const double yScale = PLOT_HEIGHT / (iaMax - 0.0);

            const double sx1 = (vaMin  - 0.0) * xScale;
            const double sy1 = PLOT_HEIGHT - (iaHigh - 0.0) * yScale;
            const double sx2 = (vaMax2 - 0.0) * xScale;
            const double sy2 = PLOT_HEIGHT - (iaLow  - 0.0) * yScale;
            const double sx3 = sx2;
            const double sy3 = PLOT_HEIGHT; // Ia = 0
            const double sx4 = sx1;
            const double sy4 = sy3;

            QPolygonF poly;
            poly << QPointF(sx1, sy1)
                 << QPointF(sx2, sy2)
                 << QPointF(sx3, sy3)
                 << QPointF(sx4, sy4);

            auto *polyItem = new QGraphicsPolygonItem(poly);
            QColor fillColor = QColor::fromRgb(0, 0, 255);
            fillColor.setAlpha(40);
            polyItem->setBrush(fillColor);
            polyItem->setPen(Qt::NoPen);
            anodeLoadLine->addToGroup(polyItem);

            if (!anodeLoadLine->childItems().isEmpty()) {
                plot->getScene()->addItem(anodeLoadLine);
            } else {
                delete anodeLoadLine;
                anodeLoadLine = nullptr;
            }
        }
    }

    // Max swing (brown) and symmetric swing (blue) tick/label helpers along AC load line
    {
        const double slope = gradient; // mA/V, same as AC load line
        const double ia0   = ia;
        const double va0   = vb;

        auto ia_line_mA = [&](double va) {
            return ia0 + slope * (va - va0);
        };

        // Left limit: intersection of AC load line with Vg1 = 0 curve at current screen voltage
        double vaLeft = -1.0;
        if (device1) {
            auto f = [&](double va) {
                double ia_curve_mA = device1->anodeCurrent(va, 0.0, vs) * 1000.0;
                double ia_line = ia_line_mA(va);
                return ia_curve_mA - ia_line;
            };

            const int samples = 400;
            double lastVa = std::clamp(va0, 0.0, vaMax);
            double lastF  = f(lastVa);
            for (int i = 1; i <= samples; ++i) {
                double va = va0 * (1.0 - static_cast<double>(i) / samples);
                va = std::max(va, 0.0);
                double curF = f(va);
                if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                    double denom = (curF - lastF);
                    double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                    t = std::clamp(t, 0.0, 1.0);
                    vaLeft = lastVa + t * (va - lastVa);
                    break;
                }
                lastVa = va;
                lastF  = curF;
            }
        }

        // Right limits: Ia = 0 and optional Pa max
        double vaRight = -1.0;
        if (vaLeft >= 0.0) {
            // Ia = 0 crossing
            double vaZero = va0 - ia0 / slope;
            vaZero = std::clamp(vaZero, 0.0, vaMax);

            double vaPa = vaMax + 1.0;
            const double paMaxW = device1 ? device1->getPaMax() : 0.0;
            if (device1 && paMaxW > 0.0) {
                auto g_pa = [&](double va) {
                    if (va <= 0.0) return 1e9;
                    double ia_line = ia_line_mA(va);
                    double ia_pa_mA = 1000.0 * paMaxW / va;
                    return ia_line - ia_pa_mA;
                };
                const int samples = 400;
                double lastVa = std::max(va0, 1e-3);
                double lastF  = g_pa(lastVa);
                for (int i = 1; i <= samples; ++i) {
                    double va = va0 + (vaMax - va0) * (static_cast<double>(i) / samples);
                    double curF = g_pa(va);
                    if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                        double denom = (curF - lastF);
                        double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                        t = std::clamp(t, 0.0, 1.0);
                        vaPa = lastVa + t * (va - lastVa);
                        break;
                    }
                    lastVa = va;
                    lastF  = curF;
                }
            }

            vaRight = std::min(vaZero, vaPa);
            vaRight = std::clamp(vaRight, 0.0, vaMax);
        }

        if (vaLeft >= 0.0 && vaRight > va0 && vaLeft < va0) {
            // Max swing (brown): span between vaLeft and vaRight
            const double Vpp_max = vaRight - vaLeft;
            const double midMax  = 0.5 * (vaLeft + vaRight);

            QGraphicsItemGroup *maxSwingGroup = new QGraphicsItemGroup();
            QPen maxPen(QColor::fromRgb(165, 42, 42)); // brown
            maxPen.setWidth(2);

            // Vertical ticks up to the AC line
            const double iaLeft  = ia_line_mA(vaLeft);
            const double iaRight = ia_line_mA(vaRight);
            if (auto *lt = plot->createSegment(vaLeft, 0.0, vaLeft, iaLeft, maxPen)) {
                maxSwingGroup->addToGroup(lt);
            }
            if (auto *rt = plot->createSegment(vaRight, 0.0, vaRight, iaRight, maxPen)) {
                maxSwingGroup->addToGroup(rt);
            }

            // Labels at tick positions
            const double labelRow = -yMajor * 1.8;
            if (auto *lLbl = plot->createLabel(vaLeft, labelRow, vaLeft, maxPen.color())) {
                QPointF p = lLbl->pos();
                double w = lLbl->boundingRect().width();
                lLbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                maxSwingGroup->addToGroup(lLbl);
            }
            if (auto *rLbl = plot->createLabel(vaRight, labelRow, vaRight, maxPen.color())) {
                QPointF p = rLbl->pos();
                double w = rLbl->boundingRect().width();
                rLbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                maxSwingGroup->addToGroup(rLbl);
            }

            // Centered Vpp_max label
            if (auto *lbl = plot->createLabel(midMax, labelRow, Vpp_max, maxPen.color())) {
                QPointF p = lbl->pos();
                double w = lbl->boundingRect().width();
                lbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                maxSwingGroup->addToGroup(lbl);
            }

            // Attach to existing anodeLoadLine group if present, otherwise create one
            if (!anodeLoadLine) {
                anodeLoadLine = new QGraphicsItemGroup();
            }
            anodeLoadLine->addToGroup(maxSwingGroup);
            if (!plot->getScene()->items().contains(anodeLoadLine)) {
                plot->getScene()->addItem(anodeLoadLine);
            }

            // Symmetric swing (blue): around the operating point, shown only
            // when the Max Sym Swing checkbox is enabled.
            const double vpk_sym = std::min(va0 - vaLeft, vaRight - va0);
            if (showSymSwing && vpk_sym > 0.0) {
                const double leftX  = va0 - vpk_sym;
                const double rightX = va0 + vpk_sym;

                QGraphicsItemGroup *symSwingGroup = new QGraphicsItemGroup();
                QPen symPen(QColor::fromRgb(100, 149, 237)); // light blue
                symPen.setWidth(2);

                const double iaLeftSym  = ia_line_mA(leftX);
                const double iaRightSym = ia_line_mA(rightX);
                if (auto *lt = plot->createSegment(leftX, 0.0, leftX, iaLeftSym, symPen)) {
                    symSwingGroup->addToGroup(lt);
                }
                if (auto *rt = plot->createSegment(rightX, 0.0, rightX, iaRightSym, symPen)) {
                    symSwingGroup->addToGroup(rt);
                }

                const double labelRowSym = -yMajor * 2.4;
                const QColor symColor = symPen.color();
                if (auto *lLbl = plot->createLabel(leftX, labelRowSym, leftX, symColor)) {
                    QPointF p = lLbl->pos();
                    double w = lLbl->boundingRect().width();
                    lLbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                    symSwingGroup->addToGroup(lLbl);
                }
                if (auto *rLbl = plot->createLabel(rightX, labelRowSym, rightX, symColor)) {
                    QPointF p = rLbl->pos();
                    double w = rLbl->boundingRect().width();
                    rLbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                    symSwingGroup->addToGroup(rLbl);
                }

                const double Vpp_sym = 2.0 * vpk_sym;
                if (auto *lbl = plot->createLabel(va0, labelRowSym, Vpp_sym, symColor)) {
                    QPointF p = lbl->pos();
                    double w = lbl->boundingRect().width();
                    lbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                    symSwingGroup->addToGroup(lbl);
                }

                anodeLoadLine->addToGroup(symSwingGroup);
                if (!plot->getScene()->items().contains(anodeLoadLine)) {
                    plot->getScene()->addItem(anodeLoadLine);
                }
            }
        }
    }
}
