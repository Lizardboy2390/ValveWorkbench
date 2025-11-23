#include "singleendeduloutput.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>

SingleEndedUlOutput::SingleEndedUlOutput()
{
    // Input parameters mirroring web SingleEndedUL defaults
    parameter[SEUL_VB]  = new Parameter("Supply voltage (V)", 300.0);
    parameter[SEUL_TAP] = new Parameter("Screen tap", 0.5);
    parameter[SEUL_IA]  = new Parameter("Bias current (anode) (mA)", 30.0);
    parameter[SEUL_RA]  = new Parameter("Anode load (\u03a9)", 8000.0);

    // Calculated values
    parameter[SEUL_HEADROOM] = new Parameter("Headroom at anode (Vpk)", 0.0);
    parameter[SEUL_VK]       = new Parameter("Bias point Vk (V)", 0.0);
    parameter[SEUL_IK]       = new Parameter("Cathode current (mA)", 0.0);
    parameter[SEUL_RK]       = new Parameter("Cathode resistor (\u03a9)", 0.0);
    parameter[SEUL_POUT]     = new Parameter("Max output power (W)", 0.0);
    parameter[SEUL_PHEAD]    = new Parameter("Power at headroom (W)", 0.0);
    parameter[SEUL_HD2]      = new Parameter("2nd harmonic (%)", 0.0);
    parameter[SEUL_HD3]      = new Parameter("3rd harmonic (%)", 0.0);
    parameter[SEUL_HD4]      = new Parameter("4th harmonic (%)", 0.0);
    parameter[SEUL_THD]      = new Parameter("Total harmonic (%)", 0.0);
}

int SingleEndedUlOutput::getDeviceType(int index)
{
    Q_UNUSED(index);
    // Prefer pentode devices for SE UL output stage
    return PENTODE;
}

QTreeWidgetItem *SingleEndedUlOutput::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void SingleEndedUlOutput::setGainMode(int mode)
{
    gainMode = mode ? 1 : 0;
    update(SEUL_HEADROOM);
}

void SingleEndedUlOutput::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first 5 fields (including manual Headroom)
    for (int i = 0; i <= SEUL_HEADROOM; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 2));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);

            if (i == SEUL_HEADROOM) {
                const double headroomManual = parameter[SEUL_HEADROOM]->getValue();
                const bool overrideActive   = (headroomManual > 0.0);
                QString style;
                if (overrideActive) {
                    // Manual override for UL headroom: bright blue, mirroring SE behaviour.
                    style = "color: rgb(0,0,255);";
                }
                labels[i]->setStyleSheet(style);
                values[i]->setStyleSheet(style);
            } else {
                labels[i]->setStyleSheet("");
                values[i]->setStyleSheet("");
            }
        }
    }

    const double headroomManual = parameter[SEUL_HEADROOM]->getValue();
    const bool overrideActive   = (headroomManual > 0.0);

    // Outputs: SEUL_VK..SEUL_THD
    for (int i = SEUL_VK; i <= SEUL_THD; ++i) {
        if (!labels[i] || !values[i]) continue;

        QString labelText;
        switch (i) {
        case SEUL_VK:    labelText = "Bias point Vk (V):"; break;
        case SEUL_IK:    labelText = "Cathode current (mA):"; break;
        case SEUL_RK:    labelText = "Cathode resistor (\u03a9):"; break;
        case SEUL_POUT:  labelText = "Max output power (W):"; break;
        case SEUL_PHEAD: labelText = "Power at headroom (W):"; break;
        case SEUL_HD2:   labelText = "2nd harmonic (%):"; break;
        case SEUL_HD3:   labelText = "3rd harmonic (%):"; break;
        case SEUL_HD4:   labelText = "4th harmonic (%):"; break;
        case SEUL_THD:   labelText = "Total harmonic (%):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = 3;
            if (i == SEUL_POUT || i == SEUL_PHEAD || i == SEUL_HD2 || i == SEUL_HD3 || i == SEUL_HD4 || i == SEUL_THD) {
                decimals = 2;
            }
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);

        // Colour distortion-related outputs when manual headroom is active.
        if (i == SEUL_PHEAD || i == SEUL_HD2 || i == SEUL_HD3 || i == SEUL_HD4 || i == SEUL_THD) {
            if (overrideActive) {
                values[i]->setStyleSheet("color: rgb(0,0,255);");
            } else {
                values[i]->setStyleSheet("");
            }
        } else {
            values[i]->setStyleSheet("");
        }
    }

    // Input sensitivity (Vpp) field after SEUL_THD
    const int sensIndex = SEUL_THD + 1;
    if (sensIndex < 16 && labels[sensIndex] && values[sensIndex]) {
        labels[sensIndex]->setText("Input sensitivity (Vpp):");
        if (inputSensitivityVpp > 0.0) {
            values[sensIndex]->setText(QString::number(inputSensitivityVpp, 'f', 2));
        } else {
            values[sensIndex]->setText("");
        }
        labels[sensIndex]->setVisible(true);
        values[sensIndex]->setVisible(true);
        values[sensIndex]->setReadOnly(true);

        if (overrideActive && inputSensitivityVpp > 0.0) {
            values[sensIndex]->setStyleSheet("color: rgb(0,0,255);");
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

QPointF SingleEndedUlOutput::findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
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

double SingleEndedUlOutput::dcLoadlineCurrent(double vb, double raa, double va) const
{
    const double q = vb / raa;
    const double m = -q / vb;
    return m * va + q;
}

double SingleEndedUlOutput::findGridBiasForCurrent(double targetIa_A,
                                                    double vb,
                                                    double tap,
                                                    double raa) const
{
    const double va = vb - targetIa_A * raa;
    if (va <= 0.0 || !std::isfinite(va)) {
        return 0.0;
    }

    const double vg1Max = device1->getVg1Max() * 2.0;
    double bestVg1 = 0.0;
    double minErr = std::numeric_limits<double>::infinity();
    const int vgSteps = 400;

    const double vsBias = va * tap + vb * (1.0 - tap);

    for (int i = 0; i <= vgSteps; ++i) {
        const double vg1 = vg1Max * static_cast<double>(i) / vgSteps;
        const double iaTest = device1->anodeCurrent(va, -vg1, vsBias);
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

double SingleEndedUlOutput::findVaFromVg(double vg1,
                                         double vb,
                                         double tap,
                                         double raa) const
{
    double va = 0.0;
    double incr = vb / 10.0;

    for (;;) {
        const double iaLine = dcLoadlineCurrent(vb, raa, va);
        const double vs = va * tap + vb * (1.0 - tap);
        const double it = device1->anodeCurrent(va, -vg1, vs);

        if (!std::isfinite(it) || !std::isfinite(iaLine)) {
            break;
        }

        if (it >= iaLine && incr <= 1e-6) {
            break;
        } else if (it >= iaLine) {
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

bool SingleEndedUlOutput::computeHeadroomHarmonicCurrents(double vb,
                                                           double ia_mA,
                                                           double raa,
                                                           double headroom,
                                                           double tap,
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

    const double kMinVa = 1e-3;
    const double kMaxVa = 2.0 * vb;
    vMin = std::max(vMin, kMinVa);
    vMax = std::clamp(vMax, vMin + 1e-6, kMaxVa);

    const double I_max = dcLoadlineCurrent(vb, raa, vMin);
    const double I_min = dcLoadlineCurrent(vb, raa, vMax);
    if (!std::isfinite(I_max) || !std::isfinite(I_min)) {
        return false;
    }

    const double Vg_bias = findGridBiasForCurrent(biasCurrent_A, vb, tap, raa);
    const double Vg_max  = findGridBiasForCurrent(I_max,          vb, tap, raa);

    const double Vg_max_mid = Vg_bias + (Vg_max - Vg_bias) / 2.0;
    const double Vg_min_mid = Vg_bias - (Vg_max - Vg_bias) / 2.0;
    const double Vg_min     = Vg_bias - (Vg_max - Vg_bias);

    const double V_min_mid_distorted = findVaFromVg(Vg_max_mid, vb, tap, raa);
    const double V_max_mid_distorted = findVaFromVg(Vg_min_mid, vb, tap, raa);
    const double V_max_distorted     = findVaFromVg(Vg_min,     vb, tap, raa);

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

bool SingleEndedUlOutput::simulateHarmonicsTimeDomain(double vb,
                                                      double iaBias_mA,
                                                      double raa,
                                                      double headroomVpk,
                                                      double tap,
                                                      double &hd2,
                                                      double &hd3,
                                                      double &hd4,
                                                      double &thd) const
{
    hd2 = hd3 = hd4 = thd = 0.0;

    if (!device1) {
        return false;
    }

    if (vb <= 0.0 || raa <= 0.0 || iaBias_mA <= 0.0 || headroomVpk <= 0.0) {
        return false;
    }

    double Ia = 0.0;
    double Ib = 0.0;
    double Ic = 0.0;
    double Id = 0.0;
    double Ie = 0.0;
    if (!computeHeadroomHarmonicCurrents(vb,
                                         iaBias_mA,
                                         raa,
                                         headroomVpk,
                                         tap,
                                         Ia,
                                         Ib,
                                         Ic,
                                         Id,
                                         Ie)) {
        return false;
    }

    double samples[5];
    samples[0] = Ia;
    samples[1] = Ib;
    samples[2] = Ic;
    samples[3] = Id;
    samples[4] = Ie;

    const int sampleCount = 512;
    const double twoPi = 6.28318530717958647692; // 2 * pi

    double a[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    double b[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

    for (int k = 0; k < sampleCount; ++k) {
        const double phase = twoPi * static_cast<double>(k) / static_cast<double>(sampleCount);
        const double u = phase / twoPi;

        const double pos = u * 5.0;
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

        const double window = 0.5 * (1.0 - std::cos(twoPi * static_cast<double>(k) /
                                                   static_cast<double>(sampleCount - 1)));
        const double v = ip * window;

        for (int n = 1; n <= 4; ++n) {
            const double angle = static_cast<double>(n) * phase;
            const double c = std::cos(angle);
            const double s = std::sin(angle);
            a[n] += v * c;
            b[n] += v * s;
        }
    }

    const double scale = 2.0 / static_cast<double>(sampleCount);
    double A[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    for (int n = 1; n <= 4; ++n) {
        a[n] *= scale;
        b[n] *= scale;
        A[n] = std::sqrt(a[n] * a[n] + b[n] * b[n]);
    }

    const double fundamental = A[1];
    if (!(fundamental > 0.0) || !std::isfinite(fundamental)) {
        return false;
    }

    const double invFund = 100.0 / fundamental;
    hd2 = A[2] * invFund;
    hd3 = A[3] * invFund;
    hd4 = A[4] * invFund;

    if (!std::isfinite(hd2) || hd2 < 0.0) hd2 = 0.0;
    if (!std::isfinite(hd3) || hd3 < 0.0) hd3 = 0.0;
    if (!std::isfinite(hd4) || hd4 < 0.0) hd4 = 0.0;

    thd = std::sqrt(hd2 * hd2 + hd3 * hd3 + hd4 * hd4);
    if (!std::isfinite(thd) || thd < 0.0) {
        thd = 0.0;
    }

    return true;
}

void SingleEndedUlOutput::update(int index)
{
    Q_UNUSED(index);

    if (!device1) {
        parameter[SEUL_VK]->setValue(0.0);
        parameter[SEUL_IK]->setValue(0.0);
        parameter[SEUL_RK]->setValue(0.0);
        parameter[SEUL_POUT]->setValue(0.0);
        parameter[SEUL_PHEAD]->setValue(0.0);
        parameter[SEUL_HD2]->setValue(0.0);
        parameter[SEUL_HD3]->setValue(0.0);
        parameter[SEUL_HD4]->setValue(0.0);
        parameter[SEUL_THD]->setValue(0.0);
        return;
    }

    const double vb  = parameter[SEUL_VB]->getValue();
    const double tap = parameter[SEUL_TAP]->getValue();
    const double ia  = parameter[SEUL_IA]->getValue(); // mA
    const double raa = parameter[SEUL_RA]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        parameter[SEUL_VK]->setValue(0.0);
        parameter[SEUL_IK]->setValue(0.0);
        parameter[SEUL_RK]->setValue(0.0);
        parameter[SEUL_POUT]->setValue(0.0);
        parameter[SEUL_PHEAD]->setValue(0.0);
        parameter[SEUL_HD2]->setValue(0.0);
        parameter[SEUL_HD3]->setValue(0.0);
        parameter[SEUL_HD4]->setValue(0.0);
        parameter[SEUL_THD]->setValue(0.0);
        return;
    }

    const double vg1Max = device1->getVg1Max() * 2.0;
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

    // Anode characteristics at Vg1=0, Vg2 from UL tap
    QVector<QPointF> anodeCurve0;
    for (int i = 1; i < 101; ++i) {
        const double va = vaMax * static_cast<double>(i) / 100.0;
        const double vg2 = va * tap + vb * (1.0 - tap);
        double ia0_mA = device1->anodeCurrent(va, 0.0, vg2) * 1000.0;
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

    // Find Vk (grid bias) such that Ia(Vb, -Vk, Vg2=Vb) ~= ia
    double bestVg1 = 0.0;
    double minErr = std::numeric_limits<double>::infinity();
    const int vgSteps = 100;
    for (int i = 0; i <= vgSteps; ++i) {
        const double vg1 = vg1Max * static_cast<double>(i) / vgSteps;
        double ia_test_mA = device1->anodeCurrent(vb, -vg1, vb) * 1000.0;
        if (!std::isfinite(ia_test_mA)) continue;
        const double err = std::abs(ia - ia_test_mA);
        if (err < minErr) {
            minErr = err;
            bestVg1 = vg1;
        }
    }

    double ik_mA = ia;
    if (device1->getDeviceType() == PENTODE) {
        // Device::screenCurrent returns mA, so add it directly without
        // additional 1000x conversion.
        ik_mA += device1->screenCurrent(vb, -bestVg1, vb);
    }

    double rk_ohms = 0.0;
    if (ik_mA > 0.0) {
        rk_ohms = 1000.0 * bestVg1 / ik_mA;
    }

    parameter[SEUL_VK]->setValue(bestVg1);
    parameter[SEUL_IK]->setValue(ik_mA);
    parameter[SEUL_RK]->setValue(rk_ohms);
    parameter[SEUL_POUT]->setValue(pout_W);

    // Headroom / harmonics / sensitivity
    effectiveHeadroomVpk = 0.0;
    inputSensitivityVpp  = 0.0;
    double phead_W = 0.0;
    double hd2 = 0.0;
    double hd3 = 0.0;
    double hd4 = 0.0;
    double thd = 0.0;

    const double headroom = parameter[SEUL_HEADROOM]->getValue();
    if (headroom > 0.0 && raa > 0.0) {
        effectiveHeadroomVpk = headroom;
        // Single-ended UL behaves like SE for power at headroom with respect to the
        // anode load.
        phead_W = (effectiveHeadroomVpk * effectiveHeadroomVpk) / (2.0 * raa);

        if (simulateHarmonicsTimeDomain(vb,
                                        ia,
                                        raa,
                                        effectiveHeadroomVpk,
                                        tap,
                                        hd2,
                                        hd3,
                                        hd4,
                                        thd)) {

            // If cathode is unbypassed (gainMode == 0), approximate the effect
            // of local feedback by reducing harmonic amplitudes by (1 + gm*Rk)
            // based on a small-signal gm estimate around the operating point.
            if (gainMode == 0 && rk_ohms > 0.0) {
                const double ia_A   = ia / 1000.0;
                const double vaBias = vb - ia_A * raa;
                const double vgBias = -bestVg1;
                const double vsBias = vaBias * tap + vb * (1.0 - tap);
                const double dVg    = std::max(0.05, std::abs(vgBias) * 0.02);

                double iaPlus_mA  = device1->anodeCurrent(vaBias, vgBias + dVg, vsBias);
                double iaMinus_mA = device1->anodeCurrent(vaBias, vgBias - dVg, vsBias);
                double gm_mA_per_V = 0.0;
                if (std::isfinite(iaPlus_mA) && std::isfinite(iaMinus_mA) && dVg > 0.0) {
                    gm_mA_per_V = (iaPlus_mA - iaMinus_mA) / (2.0 * dVg);
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
        }
    }

    parameter[SEUL_PHEAD]->setValue(phead_W);
    parameter[SEUL_HD2]->setValue(hd2);
    parameter[SEUL_HD3]->setValue(hd3);
    parameter[SEUL_HD4]->setValue(hd4);
    parameter[SEUL_THD]->setValue(thd);

    // Input sensitivity: approximate gain from small-signal gm·Ra around the UL
    // operating point, respecting K-bypass.
    double vppIn = 0.0;
    if (device1 && effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        const double ia_A   = ia / 1000.0;
        const double vaBias = vb - ia_A * raa;
        const double vgBias = -bestVg1;
        const double vsBias = vaBias * tap + vb * (1.0 - tap);
        const double dVg    = std::max(0.05, std::abs(vgBias) * 0.02);

        const double Vpp = 2.0 * effectiveHeadroomVpk;

        double iaPlus  = device1->anodeCurrent(vaBias, vgBias + dVg, vsBias);
        double iaMinus = device1->anodeCurrent(vaBias, vgBias - dVg, vsBias);
        double gm_mA_per_V = 0.0;
        if (std::isfinite(iaPlus) && std::isfinite(iaMinus) && dVg > 0.0) {
            // anodeCurrent returns mA, so this is directly gm in mA/V.
            gm_mA_per_V = (iaPlus - iaMinus) / (2.0 * dVg);
        }

        double gain = 0.0;
        if (std::isfinite(gm_mA_per_V) && raa > 0.0) {
            const double gm_A_per_V = gm_mA_per_V / 1000.0;
            gain = std::abs(gm_A_per_V * raa);

            if (gainMode == 0 && rk_ohms > 0.0) {
                const double feedback = 1.0 + gm_A_per_V * rk_ohms;
                if (feedback > 1.0 && std::isfinite(feedback)) {
                    gain /= feedback;
                }
            }
        }

        if (std::isfinite(gain) && gain > 1e-6) {
            vppIn = Vpp / gain;
        }
    }

    if (vppIn > 0.0) {
        inputSensitivityVpp = vppIn;
    } else {
        inputSensitivityVpp = 0.0;
    }
}

void SingleEndedUlOutput::plot(Plot *plot)
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

    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, vaMax, xMajor, 0.0, iaMax, yMajor);
    }

    const double vb  = parameter[SEUL_VB]->getValue();
    const double ia  = parameter[SEUL_IA]->getValue();
    const double raa = parameter[SEUL_RA]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        return;
    }

    // Recreate AC load line for plotting
    const double gradient = -1000.0 / raa;
    const double iaMaxA   = ia - gradient * vb;
    const double vaMaxA   = -iaMaxA / gradient;

    QVector<QPointF> acLine;
    for (int i = 0; i < 101; ++i) {
        const double va = static_cast<double>(i) * vaMaxA / 100.0;
        const double ia2 = iaMaxA - va * 1000.0 / raa;
        acLine.push_back(QPointF(va, ia2));
    }

    // Draw AC load line (green)
    acSignalLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 127, 0));
        pen.setWidth(2);
        for (int i = 0; i < acLine.size() - 1; ++i) {
            const QPointF s = acLine[i];
            const QPointF e = acLine[i + 1];
            if (auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen)) {
                acSignalLine->addToGroup(seg);
            }
        }
        if (!acSignalLine->childItems().isEmpty()) {
            plot->getScene()->addItem(acSignalLine);
        } else {
            delete acSignalLine;
            acSignalLine = nullptr;
        }
    }

    // Mark the DC operating point at (Vb, Ia)
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
}
