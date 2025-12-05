#include "pushpulluloutput.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>

PushPullUlOutput::PushPullUlOutput()
{
    // Input parameters mirroring web PushPullUL defaults
    parameter[PPUL_VB]       = new Parameter("Supply voltage (V)", 300.0);
    parameter[PPUL_TAP]      = new Parameter("Screen tap", 0.3);
    parameter[PPUL_IA]       = new Parameter("Bias current (anode) (mA)", 30.0);
    parameter[PPUL_RAA]      = new Parameter("Anode-to-anode load (\u03a9)", 8000.0);

    // Calculated values
    parameter[PPUL_HEADROOM] = new Parameter("Headroom at anode (Vpk)", 0.0);
    parameter[PPUL_VK]       = new Parameter("Bias point Vk (V)", 0.0);
    parameter[PPUL_IK]       = new Parameter("Cathode current (mA)", 0.0);
    parameter[PPUL_RK]       = new Parameter("Cathode resistor (\u03a9)", 0.0);
    parameter[PPUL_POUT]     = new Parameter("Max output power (W)", 0.0);
    parameter[PPUL_PHEAD]    = new Parameter("Power at headroom (W)", 0.0);
    parameter[PPUL_HD2]      = new Parameter("2nd harmonic (%)", 0.0);
    parameter[PPUL_HD3]      = new Parameter("3rd harmonic (%)", 0.0);
    parameter[PPUL_HD4]      = new Parameter("4th harmonic (%)", 0.0);
    parameter[PPUL_THD]      = new Parameter("Total harmonic (%)", 0.0);
}

int PushPullUlOutput::getDeviceType(int index)
{
    Q_UNUSED(index);
    return PENTODE;
}

QTreeWidgetItem *PushPullUlOutput::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void PushPullUlOutput::setGainMode(int mode)
{
    gainMode = mode ? 1 : 0;
    update(PPUL_HEADROOM);
}

// Toggle symmetric vs max-swing helper preference for Designer overlays.
// This mirrors the SE/PP output stages; the actual visual update is driven
// by ValveWorkbench::on_symSwingCheck_stateChanged calling plot().
void PushPullUlOutput::setSymSwingEnabled(bool enabled)
{
    showSymSwing = enabled;
}

void PushPullUlOutput::setInductiveLoad(bool enabled)
{
    inductiveLoad = enabled;
    // Changing the load interpretation (inductive vs resistive) affects the
    // effective load-line geometry used for power/headroom calculations.
    // Recompute using PPUL_HEADROOM as the driver so all derived outputs
    // refresh consistently.
    update(PPUL_HEADROOM);
}

void PushPullUlOutput::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first 5 fields (including manual Headroom)
    for (int i = 0; i <= PPUL_HEADROOM; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 2));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);

            if (i == PPUL_HEADROOM) {
                const double headroomManual = parameter[PPUL_HEADROOM]->getValue();
                const bool overrideActive   = (headroomManual > 0.0);
                QString style;
                if (overrideActive) {
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

    const double headroomManual = parameter[PPUL_HEADROOM]->getValue();
    const bool overrideActive   = (headroomManual > 0.0);

    // Outputs: PPUL_VK..PPUL_THD
    for (int i = PPUL_VK; i <= PPUL_THD; ++i) {
        if (!labels[i] || !values[i]) continue;

        QString labelText;
        switch (i) {
        case PPUL_VK:    labelText = "Bias point Vk (V):"; break;
        case PPUL_IK:    labelText = "Cathode current (mA):"; break;
        case PPUL_RK:    labelText = "Cathode resistor (\u03a9):"; break;
        case PPUL_POUT:  labelText = "Max output power (W):"; break;
        case PPUL_PHEAD: labelText = "Power at headroom (W):"; break;
        case PPUL_HD2:   labelText = "2nd harmonic (%):"; break;
        case PPUL_HD3:   labelText = "3rd harmonic (%):"; break;
        case PPUL_HD4:   labelText = "4th harmonic (%):"; break;
        case PPUL_THD:   labelText = "Total harmonic (%):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = 3;
            if (i == PPUL_POUT || i == PPUL_PHEAD || i == PPUL_HD2 || i == PPUL_HD3 || i == PPUL_HD4 || i == PPUL_THD) {
                decimals = 2;
            }
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);

        if (i == PPUL_PHEAD || i == PPUL_HD2 || i == PPUL_HD3 || i == PPUL_HD4 || i == PPUL_THD) {
            if (overrideActive) {
                values[i]->setStyleSheet("color: rgb(0,0,255);");
            } else {
                values[i]->setStyleSheet("");
            }
        } else {
            values[i]->setStyleSheet("");
        }
    }

    const int sensIndex = PPUL_THD + 1;
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

QPointF PushPullUlOutput::findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
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

void PushPullUlOutput::update(int index)
{
    Q_UNUSED(index);

    if (!device1) {
        parameter[PPUL_VK]->setValue(0.0);
        parameter[PPUL_IK]->setValue(0.0);
        parameter[PPUL_RK]->setValue(0.0);
        parameter[PPUL_POUT]->setValue(0.0);
        return;
    }

    const double vb  = parameter[PPUL_VB]->getValue();
    const double tap = parameter[PPUL_TAP]->getValue();
    const double ia  = parameter[PPUL_IA]->getValue(); // mA per tube
    const double raa = parameter[PPUL_RAA]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        parameter[PPUL_VK]->setValue(0.0);
        parameter[PPUL_IK]->setValue(0.0);
        parameter[PPUL_RK]->setValue(0.0);
        parameter[PPUL_POUT]->setValue(0.0);
        return;
    }

    const double vg1Max = device1->getVg1Max() * 2.0;
    const double vaMax  = device1->getVaMax();

    // Class B load line (not plotted here, but used conceptually)
    const double iaMaxB = 4000.0 * vb / raa;
    QVector<QPointF> classBLine;
    classBLine.push_back(QPointF(0.0, iaMaxB));
    classBLine.push_back(QPointF(vb, 0.0));

    // AC load line combining class B and A regions. In inductive mode, keep
    // the original VTADIY-style smoothed combination pivoting around Va=Vb.
    // In resistive mode, approximate a classic per-valve resistive load line
    // for RAA/2, from (0, 2*Vb/RAA) to (Vb, 0).
    const double gradient = -2000.0 / raa;          // mA/V
    QVector<QPointF> acLine;
    if (inductiveLoad) {
        const double iaMaxA   = ia - gradient * vb;     // Ia at Va = 0 for class A segment
        const double vaMaxA   = -iaMaxA / gradient;     // Va intercept

        for (int i = 0; i < 101; ++i) {
            const double va = static_cast<double>(i) * vaMaxA / 100.0;
            const double ia1 = iaMaxB - va * 4000.0 / raa;
            const double ia2 = iaMaxA - va * 2000.0 / raa;
            const double k = 5.0;
            const double r = std::exp(-ia1 / k) + std::exp(-ia2 / k);
            const double ia_max = -k * std::log(r);
            acLine.push_back(QPointF(va, ia_max));
        }
    } else {
        // Resistive per-valve load line: R = RAA/2, so Ia(0) = 2*Vb/RAA.
        const double rPerValve = raa / 2.0;
        double ia0_mA = 0.0;
        if (rPerValve > 0.0) {
            ia0_mA = (vb * 1000.0) / rPerValve; // 2*Vb/RAA in mA
        }
        const double vaStop = vb;

        for (int i = 0; i < 101; ++i) {
            const double va = static_cast<double>(i) * vaStop / 100.0;
            const double ia2 = ia0_mA + gradient * va;
            acLine.push_back(QPointF(va, ia2));
        }
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
    double minErr  = std::numeric_limits<double>::infinity();
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
        // Device::screenCurrent already returns mA, so no extra 1000x factor
        // is needed when forming Ik.
        ik_mA += device1->screenCurrent(vb, -bestVg1, vb);
    }

    double rk_ohms = 0.0;
    if (ik_mA > 0.0) {
        rk_ohms = 1000.0 * bestVg1 / (ik_mA * 2.0);
    }

    parameter[PPUL_VK]->setValue(bestVg1);
    parameter[PPUL_IK]->setValue(ik_mA);
    parameter[PPUL_RK]->setValue(rk_ohms);
    parameter[PPUL_POUT]->setValue(pout_W);

    // Headroom / harmonics / sensitivity (UL PP parity with PP).
    effectiveHeadroomVpk = 0.0;
    inputSensitivityVpp  = 0.0;
    double phead_W = 0.0;
    double hd2 = 0.0;
    double hd3 = 0.0;
    double hd4 = 0.0;
    double thd = 0.0;

    const double headroomManual = parameter[PPUL_HEADROOM]->getValue();

    // Determine effective headroom (Vpk at anode) driving PHEAD/THD/sensitivity.
    // - If manual Headroom>0, use that directly.
    // - If Headroom==0 and swing helpers are available, use Vpp_sym/2 when
    //   showSymSwing is true, otherwise use Vpp_max/2.
    double symVpp = 0.0;
    double maxVpp = 0.0;

    if (raa > 0.0 && device1) {
        const double slope = -2000.0 / raa; // mA/V, same as AC load line
        const double ia0   = ia;
        double va0   = vb;

        // For a resistive load, each valve effectively sees RAA/2 at DC, so
        // bias lies near Va_bias = Vb - Ia * (RAA/2).
        if (!inductiveLoad) {
            const double ia_A     = ia / 1000.0;
            const double rPerValve = raa / 2.0;
            double vaBias = vb;
            if (rPerValve > 0.0 && ia_A > 0.0) {
                vaBias = vb - ia_A * rPerValve;
                if (!std::isfinite(vaBias)) {
                    vaBias = vb;
                }
            }
            va0 = std::clamp(vaBias, 0.0, vaMax);
        }

        auto ia_line_mA = [&](double va_val) {
            return ia0 + slope * (va_val - va0);
        };

        // Left limit: intersection of AC load line with Vg1=0 curve at UL tap
        // screen voltage.
        double vaLeft = -1.0;
        auto f_left = [&](double va_val) {
            const double vg2 = va_val * tap + vb * (1.0 - tap);
            double ia_curve_mA = device1->anodeCurrent(va_val, 0.0, vg2) * 1000.0;
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

        // Right limits along the AC load line: Ia = 0 crossing and optional Pa_max.
        double vaRight = -1.0;
        if (vaLeft >= 0.0) {
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

    double effective = 0.0;
    if (headroomManual > 0.0) {
        effective = headroomManual;
    } else {
        if (showSymSwing && symVpp > 0.0) {
            effective = symVpp / 2.0;
        } else if (maxVpp > 0.0) {
            effective = maxVpp / 2.0;
        }
    }
    effectiveHeadroomVpk = effective;

    if (effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        // For push-pull, each valve effectively sees a fraction of RAA, so
        // scale headroom power by ~2× compared to a simple SE helper.
        phead_W = 2.0 * (effectiveHeadroomVpk * effectiveHeadroomVpk) / raa;

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
            // of local feedback by reducing the harmonic amplitudes by (1 + gm*Rk)
            // based on a small-signal gm estimate around the operating point.
            if (gainMode == 0 && rk_ohms > 0.0) {
                const double rPerValve = raa / 2.0;
                const double ia_A      = ia / 1000.0;
                double vaBias          = vb;
                if (rPerValve > 0.0 && ia_A > 0.0) {
                    vaBias = vb - ia_A * rPerValve;
                    if (!std::isfinite(vaBias)) {
                        vaBias = vb;
                    }
                }
                const double vgBias  = -bestVg1;
                const double vsBias  = vaBias * tap + vb * (1.0 - tap);
                const double dVg     = std::max(0.05, std::abs(vgBias) * 0.02);
                double iaPlus_mA     = device1->anodeCurrent(vaBias, vgBias + dVg, vsBias);
                double iaMinus_mA    = device1->anodeCurrent(vaBias, vgBias - dVg, vsBias);
                double gm_mA_per_V   = 0.0;
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

    parameter[PPUL_PHEAD]->setValue(phead_W);
    parameter[PPUL_HD2]->setValue(hd2);
    parameter[PPUL_HD3]->setValue(hd3);
    parameter[PPUL_HD4]->setValue(hd4);
    parameter[PPUL_THD]->setValue(thd);

    // Input sensitivity: approximate gain from small-signal gm·(RAA/2) around
    // the UL operating point, respecting K-bypass.
    double vppIn = 0.0;
    if (device1 && effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        const double ia_A      = ia / 1000.0;
        const double rPerValve = raa / 2.0;
        double vaBias          = vb;
        if (rPerValve > 0.0 && ia_A > 0.0) {
            vaBias = vb - ia_A * rPerValve;
            if (!std::isfinite(vaBias)) {
                vaBias = vb;
            }
        }
        const double vgBias = -bestVg1;
        const double vsBias = vaBias * tap + vb * (1.0 - tap);
        const double dVg    = std::max(0.05, std::abs(vgBias) * 0.02);

        const double Vpp = 2.0 * effectiveHeadroomVpk;

        double iaPlus  = device1->anodeCurrent(vaBias, vgBias + dVg, vsBias);
        double iaMinus = device1->anodeCurrent(vaBias, vgBias - dVg, vsBias);
        double gm_mA_per_V = 0.0;
        if (std::isfinite(iaPlus) && std::isfinite(iaMinus) && dVg > 0.0) {
            gm_mA_per_V = (iaPlus - iaMinus) / (2.0 * dVg);
        }

        double gain = 0.0;
        if (std::isfinite(gm_mA_per_V) && raa > 0.0) {
            const double gm_A_per_V = gm_mA_per_V / 1000.0;
            gain = std::abs(gm_A_per_V * (raa / 2.0));

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

double PushPullUlOutput::dcLoadlineCurrent(double vb, double raa, double va) const
{
    const double q = vb / raa;
    const double m = -q / vb;
    return m * va + q;
}

double PushPullUlOutput::findGridBiasForCurrent(double targetIa_A,
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

double PushPullUlOutput::findVaFromVg(double vg1,
                                      double vb,
                                      double tap,
                                      double raa) const
{
    double va = 0.0;
    double incr = vb / 10.0;

    for (;;) {
        const double vs = va * tap + vb * (1.0 - tap);
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

bool PushPullUlOutput::computeHeadroomHarmonicCurrents(double vb,
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

bool PushPullUlOutput::simulateHarmonicsTimeDomain(double vb,
                                                   double iaBias_mA,
                                                   double raa,
                                                   double headroomVpk,
                                                   double tap,
                                                   double &hd2,
                                                   double &hd3,
                                                   double &hd4,
                                                   double &thd) const
{
    hd2 = 0.0;
    hd3 = 0.0;
    hd4 = 0.0;
    thd = 0.0;

    if (!device1) {
        return false;
    }

    if (vb <= 0.0 || raa <= 0.0 || iaBias_mA <= 0.0 || headroomVpk <= 0.0) {
        return false;
    }

    double Ia_base = 0.0;
    double Ib_base = 0.0;
    double Ic_base = 0.0;
    double Id_base = 0.0;
    double Ie_base = 0.0;
    if (!computeHeadroomHarmonicCurrents(vb,
                                         iaBias_mA,
                                         raa,
                                         headroomVpk,
                                         tap,
                                         Ia_base,
                                         Ib_base,
                                         Ic_base,
                                         Id_base,
                                         Ie_base)) {
        return false;
    }

    // Map the single-ended 5-point currents into a push-pull primary current
    // waveform, mirroring PushPullOutput::simulateHarmonicsTimeDomain.
    const double Ia_pp = Ia_base - Ie_base;
    const double Ib_pp = Ib_base - Id_base;
    const double Ic_pp = 0.0;
    const double Id_pp = Id_base - Ib_base;
    const double Ie_pp = Ie_base - Ia_base;

    double samples[5];
    samples[0] = Ia_pp;
    samples[1] = Ib_pp;
    samples[2] = Ic_pp;
    samples[3] = Id_pp;
    samples[4] = Ie_pp;

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
        if (i0 < 0) i0 = 0;
        if (i0 >= 5) i0 = i0 % 5;
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

void PushPullUlOutput::plot(Plot *plot)
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

    const double vb  = parameter[PPUL_VB]->getValue();
    const double tap = parameter[PPUL_TAP]->getValue();
    const double ia  = parameter[PPUL_IA]->getValue();
    const double raa = parameter[PPUL_RAA]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        return;
    }

    double axisVaMax = vaMax;
    if (vb > 0.0 && vaMax > 0.0) {
        axisVaMax = std::max(vaMax, 2.0 * vb);
    }

    const double xMajor = std::max(5.0, axisVaMax / 10.0);
    const double yMajor = std::max(0.5, iaMax / 10.0);

    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, axisVaMax, xMajor, 0.0, iaMax, yMajor);
    }

    // Shared helper label row positioned between the plot and X-axis
    // labels, consistent with other output-stage circuits.
    double labelRowHelper = -yMajor * 2.0;
    const double yScalePlot = plot->getYScale();
    if (yScalePlot > 0.0) {
        labelRowHelper = -6.0 / yScalePlot;
    }

    // Determine an effective DC anode voltage per valve when treating the
    // load as resistive. In inductive mode we keep the assumption that the
    // primary has negligible DC drop so the operating point sits at Va≈Vb.
    const double ia_A = ia / 1000.0;
    double vaBias = vb;
    if (!inductiveLoad) {
        const double rPerValve = raa / 2.0;
        if (rPerValve > 0.0 && ia_A > 0.0) {
            vaBias = vb - ia_A * rPerValve;
            if (!std::isfinite(vaBias)) {
                vaBias = vb;
            }
        }
        vaBias = std::clamp(vaBias, 0.0, axisVaMax);
    }

    // Recreate AC load line for plotting. In inductive mode, reuse the
    // smoothed class A/B combination. In resistive mode, approximate a
    // per-valve resistive line RAA/2.
    const double gradient = -2000.0 / raa;
    QVector<QPointF> acLine;
    const double iaMaxB = 4000.0 * vb / raa;
    if (inductiveLoad) {
        const double iaMaxA   = ia - gradient * vb;
        const double vaMaxA   = -iaMaxA / gradient;

        for (int i = 0; i < 101; ++i) {
            const double va = static_cast<double>(i) * vaMaxA / 100.0;
            const double ia1 = iaMaxB - va * 4000.0 / raa;
            const double ia2 = iaMaxA - va * 2000.0 / raa;
            const double k = 5.0;
            const double r = std::exp(-ia1 / k) + std::exp(-ia2 / k);
            const double ia_max = -k * std::log(r);
            acLine.push_back(QPointF(va, ia_max));
        }
    } else {
        const double rPerValve = raa / 2.0;
        double ia0_mA = 0.0;
        if (rPerValve > 0.0) {
            ia0_mA = (vb * 1000.0) / rPerValve; // 2*Vb/RAA in mA
        }
        const double vaStop = vb;

        for (int i = 0; i < 101; ++i) {
            const double va = static_cast<double>(i) * vaStop / 100.0;
            const double ia2 = ia0_mA + gradient * va;
            acLine.push_back(QPointF(va, ia2));
        }
    }

    // Draw AC load line (yellow), matching PushPullOutput small-signal line colour.
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

        const double paMaxW = device1->getPaMax();
        if (paMaxW > 0.0) {
            QPen paPen(QColor::fromRgb(255, 105, 180));
            paPen.setStyle(Qt::DashLine);
            paPen.setWidth(2);

            const double xStop = vaMax;
            const double yStop = iaMax;
            const double xEnter = std::max(1e-6, std::min(xStop,
                                      (yStop > 0.0 ? (1000.0 * paMaxW / yStop) : xStop)));

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

    // DC/Class-B reference load line for UL PP, using the same Class B concept
    // as in update(). Drawn on cathodeLoadLine so it parallels SE/PP visuals.
    if (raa > 0.0) {
        cathodeLoadLine = new QGraphicsItemGroup();
        QPen dcPen;
        dcPen.setColor(QColor::fromRgb(0, 128, 0));
        dcPen.setWidth(2);

        // Reference line from (0, Ia_maxB) to (Vb, 0) in mA space.
        const double iaMaxB_mA = 4000.0 * vb / raa;
        const double x1 = 0.0;
        const double y1 = std::clamp(iaMaxB_mA, 0.0, iaMax);
        const double x2 = std::min(vb, axisVaMax);
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

    // Mark the DC operating point on the load line
    opMarker = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(255, 0, 0));
        pen.setWidth(2);
        const double d = 5.0;
        const double vaOp = inductiveLoad ? vb : vaBias;
        if (auto *h = plot->createSegment(vaOp - d, ia, vaOp + d, ia, pen)) opMarker->addToGroup(h);
        if (auto *v = plot->createSegment(vaOp, ia - d, vaOp, ia + d, pen)) opMarker->addToGroup(v);

        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker;
            opMarker = nullptr;
        }
    }

    // Max swing (brown) and symmetric swing (blue) tick/label helpers along
    // the AC load line, mirroring the non-UL PushPullOutput behaviour. Both
    // helpers share a single label row and are mutually exclusive via
    // showSymSwing.
    {
        const double slope = -2000.0 / raa; // same as gradient
        const double ia0   = ia;
        const double va0   = vb;

        auto ia_line_mA = [&](double va) {
            return ia0 + slope * (va - va0);
        };

        // Left limit: intersection of AC load line with the Vg1=0 curve at
        // UL screen voltage.
        double vaLeft = -1.0;
        if (device1) {
            auto f = [&](double va) {
                const double vg2 = va * tap + vb * (1.0 - tap);
                double ia_curve_mA = device1->anodeCurrent(va, 0.0, vg2) * 1000.0;
                double ia_line = ia_line_mA(va);
                return ia_curve_mA - ia_line;
            };

            const int samples = 400;
            double lastVa = std::clamp(va0, 0.0, axisVaMax);
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

        // Right limits: Ia = 0 and optional Pa_max limit.
        double vaRight = -1.0;
        if (vaLeft >= 0.0) {
            // Ia = 0 crossing
            double vaZero = va0 - ia0 / slope;
            vaZero = std::clamp(vaZero, 0.0, axisVaMax);

            double vaPa = axisVaMax + 1.0;
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
                    double va = va0 + (axisVaMax - va0) * (static_cast<double>(i) / samples);
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
            vaRight = std::clamp(vaRight, 0.0, axisVaMax);
        }

        if (vaLeft >= 0.0 && vaRight > va0 && vaLeft < va0) {
            // Max swing (brown): span between vaLeft and vaRight.
            const double Vpp_max = vaRight - vaLeft;
            const double midMax  = 0.5 * (vaLeft + vaRight);

            if (!showSymSwing) {
                QGraphicsItemGroup *maxSwingGroup = new QGraphicsItemGroup();
                QPen maxPen(QColor::fromRgb(165, 42, 42)); // brown
                maxPen.setWidth(2);

                const double labelRow = labelRowHelper;

                // Vertical ticks at the swing limits down to Ia = 0.
                const double iaLeft  = ia_line_mA(vaLeft);
                const double iaRight = ia_line_mA(vaRight);
                if (auto *lt = plot->createSegment(vaLeft, 0.0, vaLeft, iaLeft, maxPen)) {
                    maxSwingGroup->addToGroup(lt);
                }
                if (auto *rt = plot->createSegment(vaRight, 0.0, vaRight, iaRight, maxPen)) {
                    maxSwingGroup->addToGroup(rt);
                }

                // Labels at tick positions on the helper row.
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

                // Centered Vpp_max label.
                if (auto *lbl = plot->createLabel(midMax, labelRow, Vpp_max, maxPen.color())) {
                    QPointF p = lbl->pos();
                    double w = lbl->boundingRect().width();
                    lbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                    maxSwingGroup->addToGroup(lbl);
                }

                if (!acSignalLine) {
                    acSignalLine = new QGraphicsItemGroup();
                    plot->getScene()->addItem(acSignalLine);
                }
                acSignalLine->addToGroup(maxSwingGroup);
            }

            // Symmetric swing (blue): around the operating point when
            // showSymSwing is enabled.
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

                const double labelRowSym = labelRowHelper;
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

                if (!acSignalLine) {
                    acSignalLine = new QGraphicsItemGroup();
                    plot->getScene()->addItem(acSignalLine);
                }
                acSignalLine->addToGroup(symSwingGroup);
            }
        }
    }
}
