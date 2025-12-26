#include "pushpulloutput.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>

#include <algorithm>
#include <cmath>
#include <limits>

PushPullOutput::PushPullOutput()
{
    parameter[PP_VB]       = new Parameter("Supply voltage (V)", 300.0);
    parameter[PP_VS]       = new Parameter("Screen voltage (V)", 250.0);
    parameter[PP_IA]       = new Parameter("Bias current (anode) (mA)", 30.0);
    parameter[PP_RAA]      = new Parameter("Anode-to-anode load (\u03a9)", 8000.0);
    parameter[PP_HEADROOM] = new Parameter("Headroom at anode (Vpk)", 0.0);

    parameter[PP_VK]    = new Parameter("Bias point Vk (V)", 0.0);
    parameter[PP_IK]    = new Parameter("Cathode current (mA)", 0.0);
    parameter[PP_RK]    = new Parameter("Cathode resistor (\u03a9)", 0.0);
    parameter[PP_POUT]  = new Parameter("Max output power (W)", 0.0);
    parameter[PP_PHEAD] = new Parameter("Power at headroom (W)", 0.0);
    parameter[PP_HD2]   = new Parameter("2nd harmonic (%)", 0.0);
    parameter[PP_HD3]   = new Parameter("3rd harmonic (%)", 0.0);
    parameter[PP_HD4]   = new Parameter("4th harmonic (%)", 0.0);
    parameter[PP_THD]   = new Parameter("Total harmonic (%)", 0.0);
}

void PushPullOutput::setGainMode(int mode)
{
    gainMode = mode ? 1 : 0;
    // Re-run update so headroom, THD and sensitivity reflect the new gain mode.
    update(PP_HEADROOM);
}

void PushPullOutput::setSymSwingEnabled(bool enabled)
{
    showSymSwing = enabled;
    // Changing which swing helper we treat as the default when Headroom==0
    // affects the effectiveHeadroomVpk used for PHEAD/THD/sensitivity, so
    // recompute using PP_HEADROOM as the driver.
    update(PP_HEADROOM);
}

void PushPullOutput::setInductiveLoad(bool enabled)
{
    inductiveLoad = enabled;
    // Changing the load interpretation (inductive vs resistive) affects the
    // effective load-line geometry used for headroom, power, and THD
    // calculations. Recompute using PP_HEADROOM as the driver so all derived
    // outputs refresh consistently.
    update(PP_HEADROOM);
}

int PushPullOutput::getDeviceType(int index)
{
    if (index == 1 || index == 2) {
        return PENTODE;
    }
    return -1;
}

QTreeWidgetItem *PushPullOutput::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void PushPullOutput::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs up to PP_HEADROOM
    for (int i = 0; i <= PP_HEADROOM; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);

            if (i == PP_HEADROOM) {
                const double headroomManual = parameter[PP_HEADROOM]->getValue();
                const bool overrideActive   = (headroomManual > 0.0);

                QString style;
                if (overrideActive) {
                    // Manual override of helper swings: bright blue.
                    style = "color: rgb(0,0,255);";
                } else if (effectiveHeadroomVpk > 0.0) {
                    // Headroom derived from helpers: lighter blue for symmetric
                    // mode, brown for max-swing mode.
                    if (showSymSwing) {
                        style = "color: rgb(100,149,237);";
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

    const double headroomManual = parameter[PP_HEADROOM]->getValue();
    const bool overrideActive   = (headroomManual > 0.0);

    // Outputs PP_VK..PP_THD
    for (int i = PP_VK; i <= PP_THD; ++i) {
        if (!labels[i] || !values[i]) continue;

        QString labelText;
        switch (i) {
        case PP_VK:   labelText = "Bias point Vk (V):"; break;
        case PP_IK:   labelText = "Cathode current (mA):"; break;
        case PP_RK:   labelText = "Cathode resistor (\u03a9):"; break;
        case PP_POUT: labelText = "Max output power (W):"; break;
        case PP_PHEAD:labelText = "Power at headroom (W):"; break;
        case PP_HD2:  labelText = "2nd harmonic (%):"; break;
        case PP_HD3:  labelText = "3rd harmonic (%):"; break;
        case PP_HD4:  labelText = "4th harmonic (%):"; break;
        case PP_THD:  labelText = "Total harmonic (%):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1 || !device2) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = 3;
            if (i == PP_POUT || i == PP_PHEAD || i == PP_HD2 || i == PP_HD3 || i == PP_HD4 || i == PP_THD) {
                decimals = 2;
            }
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);

        // Colour distortion-related outputs based on the active headroom source.
        if (i == PP_PHEAD || i == PP_HD2 || i == PP_HD3 || i == PP_HD4 || i == PP_THD) {
            if (effectiveHeadroomVpk > 0.0) {
                if (overrideActive) {
                    // Manual override: bright blue.
                    values[i]->setStyleSheet("color: rgb(0,0,255);");
                } else if (showSymSwing) {
                    // Symmetric-mode metrics: lighter blue.
                    values[i]->setStyleSheet("color: rgb(100,149,237);");
                } else {
                    // Max-swing metrics: brown.
                    values[i]->setStyleSheet("color: rgb(165,42,42);");
                }
            } else {
                values[i]->setStyleSheet("");
            }
        } else {
            values[i]->setStyleSheet("");
        }
    }

    const int sensIndex = PP_THD + 1;
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

        // Match sensitivity colour to the headroom source, mirroring SE Output.
        if (inputSensitivityVpp > 0.0 && effectiveHeadroomVpk > 0.0) {
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

    for (int i = sensIndex + 1; i < 16; ++i) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
        }
    }
}

QPointF PushPullOutput::findLineIntersection(const QPointF &line1Start, const QPointF &line1End,
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

void PushPullOutput::update(int index)
{
    Q_UNUSED(index);

    if (!device1 || !device2) {
        lastHeadroomWaveform.clear();
        lastTopTrajectory.clear();
        lastBotTrajectory.clear();
        parameter[PP_VK]->setValue(0.0);
        parameter[PP_IK]->setValue(0.0);
        parameter[PP_RK]->setValue(0.0);
        parameter[PP_POUT]->setValue(0.0);
        parameter[PP_PHEAD]->setValue(0.0);
        parameter[PP_HD2]->setValue(0.0);
        parameter[PP_HD3]->setValue(0.0);
        parameter[PP_HD4]->setValue(0.0);
        parameter[PP_THD]->setValue(0.0);
        return;
    }

    const double vb       = parameter[PP_VB]->getValue();
    const double vs       = parameter[PP_VS]->getValue();
    const double ia       = parameter[PP_IA]->getValue();
    const double raa      = parameter[PP_RAA]->getValue();
    const double headroom = parameter[PP_HEADROOM]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        lastHeadroomWaveform.clear();
        lastTopTrajectory.clear();
        lastBotTrajectory.clear();
        parameter[PP_VK]->setValue(0.0);
        parameter[PP_IK]->setValue(0.0);
        parameter[PP_RK]->setValue(0.0);
        parameter[PP_POUT]->setValue(0.0);
        parameter[PP_PHEAD]->setValue(0.0);
        parameter[PP_HD2]->setValue(0.0);
        parameter[PP_HD3]->setValue(0.0);
        parameter[PP_HD4]->setValue(0.0);
        parameter[PP_THD]->setValue(0.0);
        return;
    }

    const double vg1Max = device1->getVg1Max();
    const double vaMax  = device1->getVaMax();

    // Anode characteristics at Vg1=0, Vg2=Vs, sampled along the model's
    // Va range for estimating maximum output power.
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
        // Use an analytic AC load line to estimate maximum output power.
        // In inductive mode, keep the existing VTADIY-style line pivoting
        // around (Vb, Ia). In resistive mode, use the classic DC load line
        // for a single valve seeing RAA/2, i.e. from (0, Vb/(RAA/2)) to (Vb, 0).
        const double gradient = -2000.0 / raa; // mA/V

        QPointF aStart(0.0, 0.0);
        QPointF aEnd(vaMax, 0.0);
        if (inductiveLoad) {
            aStart = QPointF(0.0, ia - gradient * vb);
            aEnd   = QPointF(vaMax, ia + gradient * (vaMax - vb));
        } else {
            const double rPerValve = raa / 2.0;
            double ia0_mA = 0.0;
            if (rPerValve > 0.0) {
                ia0_mA = (vb * 1000.0) / rPerValve; // 2*Vb/RAA in mA
            }
            aStart = QPointF(0.0, ia0_mA);
            aEnd   = QPointF(vb, 0.0);
        }

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

    double bestVg1 = 0.0;
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
        double minErr  = std::numeric_limits<double>::infinity();
        const int vgSteps = 100;
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

        ig2_mA = 0.0;
        if (device1->getDeviceType() == PENTODE) {
            ig2_mA = device1->screenCurrent(vb, -bestVg1, vs);
        }
        ik_mA = ia + ig2_mA;
    }

    double rk_ohms = 0.0;
    if (ik_mA > 0.0) {
        rk_ohms = 1000.0 * bestVg1 / (ik_mA * 2.0);
    }

    parameter[PP_VK]->setValue(bestVg1);
    parameter[PP_IK]->setValue(ik_mA);
    parameter[PP_RK]->setValue(rk_ohms);
    parameter[PP_POUT]->setValue(pout_W);

    effectiveHeadroomVpk = 0.0;
    inputSensitivityVpp = 0.0;
    double phead_W = 0.0;
    double hd2 = 0.0;
    double hd3 = 0.0;
    double hd4 = 0.0;
    double thd = 0.0;

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

        if (!inductiveLoad) {
            // For a resistive load, each valve effectively sees RAA/2 at DC,
            // so bias lies at approximately Va_bias = Vb - Ia * (RAA/2).
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

        // Left limit: intersection of AC load line with Vg1 = 0 curve.
        double vaLeft = -1.0;
        {
            auto f_left = [&](double va_val) {
                double ia_curve_mA = device1->anodeCurrent(va_val, 0.0, vs) * 1000.0;
                double ia_line = ia_line_mA(va_val);
                return ia_curve_mA - ia_line;
            };

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
            // Ia = 0 crossing of the AC line.
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

    if (effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        // For push-pull, each valve effectively sees a fraction of RAA, so
        // scale headroom power by ~2× compared to a simple SE helper.
        phead_W = 2.0 * (effectiveHeadroomVpk * effectiveHeadroomVpk) / raa;

        if (simulateHarmonicsTimeDomain(vb,
                                        ia,
                                        raa,
                                        effectiveHeadroomVpk,
                                        vs,
                                        hd2,
                                        hd3,
                                        hd4,
                                        thd)) {

            // If cathode is unbypassed (gainMode == 0), approximate the effect
            // of local feedback by reducing the harmonic amplitudes by a simple
            // (1 + gm*Rk) factor derived from the current bias point.
            if (gainMode == 0 && rk_ohms > 0.0) {
                const double vk      = bestVg1;
                const double vgBias  = -vk;
                const double dVg     = std::max(0.05, std::abs(vgBias) * 0.02);
                double iaPlus_mA     = device1->anodeCurrent(vb, vgBias + dVg, vs);
                double iaMinus_mA    = device1->anodeCurrent(vb, vgBias - dVg, vs);
                double gm_mA_per_V   = 0.0;
                if (std::isfinite(iaPlus_mA) && std::isfinite(iaMinus_mA) && dVg > 0.0) {
                    // Device::anodeCurrent already returns mA, so the central difference
                    // directly yields gm in mA/V without additional scaling.
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

    parameter[PP_PHEAD]->setValue(phead_W);
    parameter[PP_HD2]->setValue(hd2);
    parameter[PP_HD3]->setValue(hd3);
    parameter[PP_HD4]->setValue(hd4);
    parameter[PP_THD]->setValue(thd);

    const int sensIndex = PP_THD + 1;
    double vppIn = 0.0;
    if (device1 && effectiveHeadroomVpk > 0.0 && raa > 0.0) {
        const double vbEff  = vb;
        const double vsEff  = vs;
        const double raaEff = raa;
        const double head   = effectiveHeadroomVpk;

        const double Vpp = 2.0 * head;
        const double vgBias = -bestVg1;
        const double dVg = std::max(0.05, std::abs(vgBias) * 0.02);

        double iaPlus  = device1->anodeCurrent(vbEff, vgBias + dVg, vsEff);
        double iaMinus = device1->anodeCurrent(vbEff, vgBias - dVg, vsEff);
        double gm_mA_per_V = 0.0;
        if (std::isfinite(iaPlus) && std::isfinite(iaMinus) && dVg > 0.0) {
            // Device::anodeCurrent already returns mA, so the central difference
            // directly yields gm in mA/V without additional scaling.
            gm_mA_per_V = (iaPlus - iaMinus) / (2.0 * dVg);
        }

        double gain = 0.0;
        if (std::isfinite(gm_mA_per_V) && raaEff > 0.0) {
            const double gm_A_per_V = gm_mA_per_V / 1000.0;
            gain = std::abs(gm_A_per_V * (raaEff / 2.0));

            // If cathode is unbypassed, include local feedback from Rk:
            // effective gain ≈ Av / (1 + gm*Rk).
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

double PushPullOutput::dcLoadlineCurrent(double vb, double raa, double va) const
{
    const double q = vb / raa;
    const double m = -q / vb;
    return m * va + q;
}

double PushPullOutput::findGridBiasForCurrent(double targetIa_A,
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
        const double iaTest_mA = device1->anodeCurrent(va, -vg1, vs);
        if (!std::isfinite(iaTest_mA) || iaTest_mA < 0.0) {
            continue;
        }
        const double iaTest_A = iaTest_mA / 1000.0;
        const double err = std::abs(targetIa_A - iaTest_A);
        if (err < minErr) {
            minErr = err;
            bestVg1 = vg1;
        }
    }

    return bestVg1;
}

double PushPullOutput::findVaFromVg(double vg1,
                                    double vb,
                                    double vs,
                                    double raa) const
{
    double va = 0.0;
    double incr = vb / 10.0;

    for (;;) {
        const double it_mA = device1->anodeCurrent(va, -vg1, vs);
        const double il_A  = dcLoadlineCurrent(vb, raa, va);

        if (!std::isfinite(it_mA) || !std::isfinite(il_A)) {
            break;
        }

        const double it_A = it_mA / 1000.0;

        if (it_A >= il_A && incr <= 1e-6) {
            break;
        } else if (it_A >= il_A) {
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

bool PushPullOutput::computeHeadroomHarmonicCurrents(double vb,
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

    const double rPerValve = raa / 2.0;
    if (!(rPerValve > 0.0)) {
        return false;
    }

    double vOperating = vb;
    if (!inductiveLoad) {
        vOperating = vb - biasCurrent_A * rPerValve;
    }
    if (!std::isfinite(vOperating)) {
        vOperating = vb;
    }
    vOperating = std::clamp(vOperating, 0.0, 2.0 * vb);

    double vMin = vOperating - headroom;
    double vMax = vOperating + headroom;

    const double kMinVa = 1e-3;
    const double kMaxVa = 2.0 * vb;
    vMin = std::max(vMin, kMinVa);
    vMax = std::clamp(vMax, vMin + 1e-6, kMaxVa);

    const double I_max = dcLoadlineCurrent(vb, rPerValve, vMin);
    const double I_min = dcLoadlineCurrent(vb, rPerValve, vMax);
    if (!std::isfinite(I_max) || !std::isfinite(I_min)) {
        return false;
    }

    const double Vg_bias = findGridBiasForCurrent(biasCurrent_A, vb, vs, rPerValve);
    const double Vg_max  = findGridBiasForCurrent(I_max,          vb, vs, rPerValve);

    const double Vg_max_mid = Vg_bias + (Vg_max - Vg_bias) / 2.0;
    const double Vg_min_mid = Vg_bias - (Vg_max - Vg_bias) / 2.0;
    const double Vg_min     = Vg_bias - (Vg_max - Vg_bias);

    const double V_min_mid_distorted = findVaFromVg(Vg_max_mid, vb, vs, rPerValve);
    const double V_max_mid_distorted = findVaFromVg(Vg_min_mid, vb, vs, rPerValve);
    const double V_max_distorted     = findVaFromVg(Vg_min,     vb, vs, rPerValve);

    const double I_max_mid_distorted = dcLoadlineCurrent(vb, rPerValve, V_min_mid_distorted);
    const double I_min_mid_distorted = dcLoadlineCurrent(vb, rPerValve, V_max_mid_distorted);
    const double I_min_distorted     = dcLoadlineCurrent(vb, rPerValve, V_max_distorted);

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

void PushPullOutput::computeTimeDomainHarmonicScan(QVector<double> &headroomVals,
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

    if (!device1 || !device2) {
        return;
    }

    const double vb       = parameter[PP_VB]->getValue();
    const double vs       = parameter[PP_VS]->getValue();
    const double ia       = parameter[PP_IA]->getValue();
    const double raa      = parameter[PP_RAA]->getValue();
    const double headroom = parameter[PP_HEADROOM]->getValue();

    if (!(vb > 0.0) || !(raa > 0.0) || !(ia > 0.0)) {
        return;
    }

    const double maxHeadroom = 0.9 * vb;
    const int    steps       = 32;

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
        double thd = 0.0;

        if (simulateHarmonicsTimeDomain(vb,
                                        ia,
                                        raa,
                                        head,
                                        vs,
                                        hd2,
                                        hd3,
                                        hd4,
                                        thd)) {
            headroomVals.push_back(head);
            hd2Vals.push_back(hd2);
            hd3Vals.push_back(hd3);
            hd4Vals.push_back(hd4);
            thdVals.push_back(thd);
        }
    }
}

void PushPullOutput::computeHarmonics(double Ia,
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

bool PushPullOutput::simulateHarmonicsTimeDomain(double vb,
                                                 double iaBias_mA,
                                                 double raa,
                                                 double headroomVpk,
                                                 double vs,
                                                 double &hd2,
                                                 double &hd3,
                                                 double &hd4,
                                                 double &thd) const
{
    hd2 = 0.0;
    hd3 = 0.0;
    hd4 = 0.0;
    thd = 0.0;

    if (!device1 || !device2) {
        return false;
    }

    if (vb <= 0.0 || raa <= 0.0 || iaBias_mA <= 0.0 || headroomVpk <= 0.0) {
        return false;
    }

    const double rPerValve = raa / 2.0;
    if (!(rPerValve > 0.0) || !std::isfinite(rPerValve)) {
        return false;
    }

    // Headroom parameter is specified per-anode (Vpk). The primary (anode-to-anode)
    // output swing is approximately 2x that (Vpk) for ideal symmetric operation.
    const double vprimaryVppTarget = 4.0 * headroomVpk;
    if (!(vprimaryVppTarget > 0.0) || !std::isfinite(vprimaryVppTarget)) {
        return false;
    }

    // Bias and cathode state. Vk is stored as a positive magnitude.
    const double iaBias_A = iaBias_mA / 1000.0;
    const double vk       = parameter[PP_VK] ? parameter[PP_VK]->getValue() : 0.0;
    const double rk        = parameter[PP_RK] ? parameter[PP_RK]->getValue() : 0.0;
    const double vgBias    = -vk;

    const bool   cathodeBypassed = (gainMode == 1);
    const double rk_ac           = (cathodeBypassed || rk <= 0.0) ? 0.0 : rk;

    // Estimate differential small-signal gain (primary Vaa per grid V) so we can
    // map requested headroom to an approximate grid-drive Vpp.
    double vaBias = vb;
    if (!inductiveLoad) {
        vaBias = vb - iaBias_A * rPerValve;
    }
    vaBias = std::clamp(vaBias, 0.0, 2.0 * vb);

    const double dVg = std::max(0.05, std::abs(vgBias) * 0.02);
    double gmTop_mA_per_V = 0.0;
    double gmBot_mA_per_V = 0.0;
    {
        double iaPlus_mA  = device1->anodeCurrent(vaBias, vgBias + dVg, vs);
        double iaMinus_mA = device1->anodeCurrent(vaBias, vgBias - dVg, vs);
        if (std::isfinite(iaPlus_mA) && std::isfinite(iaMinus_mA) && dVg > 0.0) {
            gmTop_mA_per_V = (iaPlus_mA - iaMinus_mA) / (2.0 * dVg);
        }
    }
    {
        double iaPlus_mA  = device2->anodeCurrent(vaBias, vgBias + dVg, vs);
        double iaMinus_mA = device2->anodeCurrent(vaBias, vgBias - dVg, vs);
        if (std::isfinite(iaPlus_mA) && std::isfinite(iaMinus_mA) && dVg > 0.0) {
            gmBot_mA_per_V = (iaPlus_mA - iaMinus_mA) / (2.0 * dVg);
        }
    }
    double gm_mA_per_V = 0.5 * (gmTop_mA_per_V + gmBot_mA_per_V);
    if (!std::isfinite(gm_mA_per_V) || std::abs(gm_mA_per_V) < 1e-6) {
        gm_mA_per_V = std::isfinite(gmTop_mA_per_V) ? gmTop_mA_per_V : gmBot_mA_per_V;
    }
    if (!std::isfinite(gm_mA_per_V) || std::abs(gm_mA_per_V) < 1e-6) {
        return false;
    }
    const double gm_A_per_V = gm_mA_per_V / 1000.0;

    double gainPrimary = 2.0 * std::abs(gm_A_per_V * rPerValve); // Vaa/Vg
    if (gainPrimary <= 1e-6 || !std::isfinite(gainPrimary)) {
        return false;
    }

    if (rk_ac > 0.0) {
        const double feedback = 1.0 + std::abs(gm_A_per_V) * rk_ac;
        if (feedback > 1.0 && std::isfinite(feedback)) {
            gainPrimary /= feedback;
        }
    }

    if (gainPrimary <= 1e-6 || !std::isfinite(gainPrimary)) {
        return false;
    }

    const double gridVpp = vprimaryVppTarget / gainPrimary;
    if (!(gridVpp > 0.0) || !std::isfinite(gridVpp)) {
        return false;
    }

    const int sampleCount = 1024;
    const double twoPi = 6.28318530717958647692;

    double a[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    double b[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

    lastHeadroomWaveform.clear();
    lastHeadroomWaveform.reserve(sampleCount);

    lastTopTrajectory.clear();
    lastBotTrajectory.clear();
    lastTopTrajectory.reserve(sampleCount);
    lastBotTrajectory.reserve(sampleCount);

    const double vaCentre = vaBias;

    const double ig2BiasTop_mA = (device1->getDeviceType() == PENTODE)
                                     ? device1->screenCurrent(vaCentre, vgBias, vs)
                                     : 0.0;
    const double ig2BiasBot_mA = (device2->getDeviceType() == PENTODE)
                                     ? device2->screenCurrent(vaCentre, vgBias, vs)
                                     : 0.0;
    const double ikBiasTotal_A = (iaBias_mA + ig2BiasTop_mA + iaBias_mA + ig2BiasBot_mA) / 1000.0;

    auto solvePrimaryVoltage = [&](double vgTop_abs, double vgBot_abs, double vk_eff, double &vPri_out,
                                   double &iaTop_mA_out, double &iaBot_mA_out,
                                   double &ig2Top_mA_out, double &ig2Bot_mA_out) -> bool {
        iaTop_mA_out = iaBot_mA_out = 0.0;
        ig2Top_mA_out = ig2Bot_mA_out = 0.0;

        auto eval = [&](double vPri) -> double {
            double vaTop = vaCentre + 0.5 * vPri;
            double vaBot = vaCentre - 0.5 * vPri;
            vaTop = std::clamp(vaTop, 0.0, 2.0 * vb);
            vaBot = std::clamp(vaBot, 0.0, 2.0 * vb);

            double vg1Top = vgTop_abs - vk_eff;
            double vg1Bot = vgBot_abs - vk_eff;
            if (vg1Top > 0.0) vg1Top = 0.0;
            if (vg1Bot > 0.0) vg1Bot = 0.0;

            const double iaTop_mA = device1->anodeCurrent(vaTop, vg1Top, vs);
            const double iaBot_mA = device2->anodeCurrent(vaBot, vg1Bot, vs);
            if (!std::isfinite(iaTop_mA) || !std::isfinite(iaBot_mA)) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            const double vLoad = raa * (iaTop_mA - iaBot_mA) / 1000.0;
            return vPri - vLoad;
        };

        // Bracket the root in a conservative range.
        double lo = -2.0 * vb;
        double hi =  2.0 * vb;
        double flo = eval(lo);
        double fhi = eval(hi);
        if (!std::isfinite(flo) || !std::isfinite(fhi)) {
            return false;
        }
        if (flo * fhi > 0.0) {
            // If not bracketed, fall back to a best-effort zero.
            lo = -vb;
            hi =  vb;
            flo = eval(lo);
            fhi = eval(hi);
            if (!std::isfinite(flo) || !std::isfinite(fhi) || flo * fhi > 0.0) {
                return false;
            }
        }

        double mid = 0.0;
        for (int it = 0; it < 30; ++it) {
            mid = 0.5 * (lo + hi);
            double fmid = eval(mid);
            if (!std::isfinite(fmid)) {
                return false;
            }
            if (std::abs(fmid) < 1e-6) {
                break;
            }
            if (flo * fmid <= 0.0) {
                hi = mid;
                fhi = fmid;
            } else {
                lo = mid;
                flo = fmid;
            }
        }

        // Final evaluation to capture currents.
        double vaTop = vaCentre + 0.5 * mid;
        double vaBot = vaCentre - 0.5 * mid;
        vaTop = std::clamp(vaTop, 0.0, 2.0 * vb);
        vaBot = std::clamp(vaBot, 0.0, 2.0 * vb);

        double vg1Top = vgTop_abs - vk_eff;
        double vg1Bot = vgBot_abs - vk_eff;
        if (vg1Top > 0.0) vg1Top = 0.0;
        if (vg1Bot > 0.0) vg1Bot = 0.0;

        iaTop_mA_out = device1->anodeCurrent(vaTop, vg1Top, vs);
        iaBot_mA_out = device2->anodeCurrent(vaBot, vg1Bot, vs);
        if (!std::isfinite(iaTop_mA_out) || !std::isfinite(iaBot_mA_out)) {
            return false;
        }

        if (device1->getDeviceType() == PENTODE) {
            ig2Top_mA_out = device1->screenCurrent(vaTop, vg1Top, vs);
        }
        if (device2->getDeviceType() == PENTODE) {
            ig2Bot_mA_out = device2->screenCurrent(vaBot, vg1Bot, vs);
        }

        vPri_out = mid;
        return true;
    };

    for (int k = 0; k < sampleCount; ++k) {
        const double phase = twoPi * static_cast<double>(k) / static_cast<double>(sampleCount);
        const double drive = 0.5 * gridVpp * std::sin(phase);

        const double vgTop_abs = drive;
        const double vgBot_abs = -drive;

        double vk_inst = vk;
        double vPri = 0.0;
        double iaTop_mA = 0.0;
        double iaBot_mA = 0.0;
        double ig2Top_mA = 0.0;
        double ig2Bot_mA = 0.0;

        if (rk_ac > 0.0) {
            bool ok = true;
            for (int iter = 0; iter < 6; ++iter) {
                if (!solvePrimaryVoltage(vgTop_abs, vgBot_abs, vk_inst, vPri, iaTop_mA, iaBot_mA, ig2Top_mA, ig2Bot_mA)) {
                    ok = false;
                    break;
                }
                const double ikTotal_A = (iaTop_mA + iaBot_mA + ig2Top_mA + ig2Bot_mA) / 1000.0;
                double vk_new = vk + (ikTotal_A - ikBiasTotal_A) * rk_ac;
                if (!std::isfinite(vk_new)) {
                    vk_new = vk;
                }
                if (vk_new < 0.0) {
                    vk_new = 0.0;
                }
                const double delta = std::abs(vk_new - vk_inst);
                vk_inst = vk_new;
                if (delta < 1e-3) {
                    break;
                }
            }
            if (!ok) {
                continue;
            }
            if (!solvePrimaryVoltage(vgTop_abs, vgBot_abs, vk_inst, vPri, iaTop_mA, iaBot_mA, ig2Top_mA, ig2Bot_mA)) {
                continue;
            }
        } else {
            if (!solvePrimaryVoltage(vgTop_abs, vgBot_abs, vk, vPri, iaTop_mA, iaBot_mA, ig2Top_mA, ig2Bot_mA)) {
                continue;
            }
        }

        const double vPrimary = vPri;
        if (std::isfinite(vPrimary)) {
            lastHeadroomWaveform.push_back(vPrimary);
        }

        const double vaTop = std::clamp(vaCentre + 0.5 * vPri, 0.0, 2.0 * vb);
        const double vaBot = std::clamp(vaCentre - 0.5 * vPri, 0.0, 2.0 * vb);
        if (std::isfinite(vaTop) && std::isfinite(iaTop_mA)) {
            lastTopTrajectory.push_back(QPointF(vaTop, std::max(0.0, iaTop_mA)));
        }
        if (std::isfinite(vaBot) && std::isfinite(iaBot_mA)) {
            lastBotTrajectory.push_back(QPointF(vaBot, std::max(0.0, iaBot_mA)));
        }

        const double window = 0.5 * (1.0 - std::cos(twoPi * static_cast<double>(k) /
                                                   static_cast<double>(sampleCount - 1)));
        const double v = vPrimary * window;

        for (int n = 1; n <= 4; ++n) {
            const double angle = static_cast<double>(n) * phase;
            a[n] += v * std::cos(angle);
            b[n] += v * std::sin(angle);
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

void PushPullOutput::plot(Plot *plot)
{
    if (!device1 || !device2) {
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
    double iaMax = device1->getIaMax();

    const double vb       = parameter[PP_VB]->getValue();
    const double vs       = parameter[PP_VS]->getValue();
    const double ia       = parameter[PP_IA]->getValue();
    const double raa      = parameter[PP_RAA]->getValue();
    const double headroom = parameter[PP_HEADROOM]->getValue();

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        return;
    }

    double axisVaMax = vaMax;
    if (vb > 0.0 && vaMax > 0.0) {
        axisVaMax = std::max(vaMax, 2.0 * vb);
    }

    const double xMajor = std::max(5.0, axisVaMax / 10.0);
    const double yMajor = std::max(0.5, iaMax / 10.0);
    double labelRowHelper = -yMajor * 2.0;
    const double yScalePlot = plot->getYScale();
    if (yScalePlot > 0.0) {
        labelRowHelper = -6.0 / yScalePlot;
    }

    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, axisVaMax, xMajor, 0.0, iaMax, yMajor);
    }

    // Determine an effective DC anode voltage per valve when treating the
    // load as resistive. In inductive mode we keep the original assumption
    // that the primary has negligible DC drop, so the operating point sits
    // at Va vb.
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

    // Recreate AC load line and its class A / class B components for plotting.
    // In inductive mode, use the original VTADIY-style geometry pivoting
    // around Va=Vb. In resistive mode, pivot the class-A component around the
    // DC bias VaVb - Ia*(RAA/2) so the helpers and operating point follow
    // the chosen load interpretation.
    const double gradient = -2000.0 / raa;
    const double vaCentre = inductiveLoad ? vb : vaBias;
    const double iaMaxA   = ia - gradient * vaCentre;
    const double vaMaxA   = -iaMaxA / gradient;

    QVector<QPointF> acLine;
    QVector<QPointF> classBLine;
    QVector<QPointF> classALine;

    const double iaMaxB = 4000.0 * vb / raa;
    for (int i = 0; i < 101; ++i) {
        const double va = static_cast<double>(i) * vaMaxA / 100.0;

        // Class B straight line component
        const double ia1 = iaMaxB - va * 4000.0 / raa;
        classBLine.push_back(QPointF(va, ia1));

        // Class A straight line component
        const double ia2 = iaMaxA - va * 2000.0 / raa;
        classALine.push_back(QPointF(va, ia2));

        // Smoothed max (approximate smax) for combined AC load line
        const double k = 5.0;
        const double r = std::exp(-ia1 / k) + std::exp(-ia2 / k);
        const double ia_max = -k * std::log(r);
        acLine.push_back(QPointF(va, ia_max));
    }

    // Class B load line (solid dark green) in anodeLoadLine
    anodeLoadLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 100, 0));
        pen.setWidth(2);
        for (int i = 0; i < classBLine.size() - 1; ++i) {
            const QPointF s = classBLine[i];
            const QPointF e = classBLine[i + 1];
            if (auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen)) {
                anodeLoadLine->addToGroup(seg);
            }
        }
        if (!anodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(anodeLoadLine);
        } else {
            delete anodeLoadLine;
            anodeLoadLine = nullptr;
        }
    }

    // Class A component line (dashed blue) in cathodeLoadLine
    cathodeLoadLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 0, 180));
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        for (int i = 0; i < classALine.size() - 1; ++i) {
            const QPointF s = classALine[i];
            const QPointF e = classALine[i + 1];
            if (auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen)) {
                cathodeLoadLine->addToGroup(seg);
            }
        }
        if (!cathodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(cathodeLoadLine);
        } else {
            delete cathodeLoadLine;
            cathodeLoadLine = nullptr;
        }
    }

    // Combined AC load line (bright green) in acSignalLine
    acSignalLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 180, 0));
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

            // Use the current Plot axes so the Pa_max curve remains visible
            // even when Designer extends the Y-range beyond the device's
            // iaMax for Class-B viewing.
            const double xScale = plot->getXScale();
            const double yScale = plot->getYScale();
            if (xScale > 0.0 && yScale > 0.0) {
                const double xStart = plot->getXStart();
                const double yStart = plot->getYStart();
                const double xStop  = xStart + static_cast<double>(PLOT_WIDTH)  / xScale;
                const double yStop  = yStart + static_cast<double>(PLOT_HEIGHT) / yScale;

                // Pa_max hyperbola: Ia = 1000 * Pa_max / Va (mA). Enter the
                // visible box where the curve first drops below the top Y
                // limit.
                const double xMaxCurve = std::max(1e-6, xStop);
                const double yTop      = yStop;
                const double xEnter    = std::max(1e-6,
                                                  std::min(xMaxCurve,
                                                           (yTop > 0.0
                                                            ? (1000.0 * paMaxW / yTop)
                                                            : xMaxCurve)));

                const int segs = 60;
                double prevX = xEnter;
                double prevY = (prevX > xStart)
                               ? std::min(yTop, 1000.0 * paMaxW / prevX)
                               : yTop;
                for (int i = 1; i <= segs; ++i) {
                    double t = static_cast<double>(i) / segs;
                    double x = xEnter + (xMaxCurve - xEnter) * t;
                    if (x < xStart) {
                        continue;
                    }
                    double y = (x > 0.0) ? std::min(yTop, 1000.0 * paMaxW / x) : yTop;
                    if (y < yStart) {
                        // Once we leave the visible box at the bottom, stop
                        // drawing further segments.
                        break;
                    }
                    if (auto *seg = plot->createSegment(prevX, prevY, x, y, paPen)) {
                        acSignalLine->addToGroup(seg);
                    }
                    prevX = x;
                    prevY = y;
                }
            }
        }

        if (!acSignalLine->childItems().isEmpty()) {
            plot->getScene()->addItem(acSignalLine);
        } else {
            delete acSignalLine;
            acSignalLine = nullptr;
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

    // Draw a headroom segment around the operating point along the AC load
    // line, with a filled polygon down to Ia = 0.
    if (effectiveHeadroomVpk > 0.0) {
        const double vaCentre = inductiveLoad ? vb : vaBias;
        const double headroomUse = effectiveHeadroomVpk;
        double vaMin = vaCentre - headroomUse;
        double vaMax2 = vaCentre + headroomUse;

        vaMin  = std::max(0.0, vaMin);
        vaMax2 = std::min(axisVaMax, vaMax2);

        if (vaMax2 > vaMin) {
            QGraphicsItemGroup *headroomGroup = new QGraphicsItemGroup();
            QPen pen;
            if (headroom > 0.0) {
                pen.setColor(QColor::fromRgb(0, 0, 255));
            } else if (showSymSwing) {
                pen.setColor(QColor::fromRgb(100, 149, 237));
            } else {
                pen.setColor(QColor::fromRgb(165, 42, 42));
            }
            pen.setWidth(2);

            auto ia_line_mA = [&](double va) {
                return ia + gradient * (va - vaCentre);
            };

            double iaHigh = ia_line_mA(vaMin);
            double iaLow  = ia_line_mA(vaMax2);

            if (std::isfinite(iaHigh) && std::isfinite(iaLow)) {
                iaHigh = std::clamp(iaHigh, 0.0, iaMax);
                iaLow  = std::clamp(iaLow,  0.0, iaMax);

                if (auto *seg = plot->createSegment(vaMin, iaHigh, vaMax2, iaLow, pen)) {
                    headroomGroup->addToGroup(seg);
                }

                const double xScale = plot->getXScale();
                const double yScale = plot->getYScale();
                const double xStart = plot->getXStart();
                const double yStart = plot->getYStart();
                if (xScale > 0.0 && yScale > 0.0) {
                    const double sx1 = (vaMin  - xStart) * xScale;
                    const double sy1 = PLOT_HEIGHT - (iaHigh - yStart) * yScale;
                    const double sx2 = (vaMax2 - xStart) * xScale;
                    const double sy2 = PLOT_HEIGHT - (iaLow  - yStart) * yScale;
                    const double sx3 = sx2;
                    const double sy3 = PLOT_HEIGHT - (0.0 - yStart) * yScale;
                    const double sx4 = sx1;
                    const double sy4 = sy3;

                    QPolygonF poly;
                    poly << QPointF(sx1, sy1)
                         << QPointF(sx2, sy2)
                         << QPointF(sx3, sy3)
                         << QPointF(sx4, sy4);

                    auto *polyItem = new QGraphicsPolygonItem(poly);
                    QColor fillColor = pen.color();
                    fillColor.setAlpha(40);
                    polyItem->setBrush(fillColor);
                    polyItem->setPen(Qt::NoPen);
                    headroomGroup->addToGroup(polyItem);
                }
            }

            if (!headroomGroup->childItems().isEmpty()) {
                // Attach to the existing AC overlay group so it is cleared on
                // subsequent replots.
                if (!acSignalLine) {
                    acSignalLine = new QGraphicsItemGroup();
                    plot->getScene()->addItem(acSignalLine);
                }
                acSignalLine->addToGroup(headroomGroup);
            } else {
                delete headroomGroup;
            }
        }
    }

    if (effectiveHeadroomVpk > 0.0 && lastTopTrajectory.size() > 1 && lastBotTrajectory.size() > 1) {
        QGraphicsItemGroup *trajGroup = new QGraphicsItemGroup();

        QPen penTop;
        penTop.setColor(QColor::fromRgb(255, 140, 0));
        penTop.setWidth(2);

        QPen penBot;
        penBot.setColor(QColor::fromRgb(160, 32, 240));
        penBot.setWidth(2);

        for (int i = 0; i < lastTopTrajectory.size() - 1; ++i) {
            const QPointF s = lastTopTrajectory[i];
            const QPointF e = lastTopTrajectory[i + 1];
            if (auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), penTop)) {
                trajGroup->addToGroup(seg);
            }
        }
        for (int i = 0; i < lastBotTrajectory.size() - 1; ++i) {
            const QPointF s = lastBotTrajectory[i];
            const QPointF e = lastBotTrajectory[i + 1];
            if (auto *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), penBot)) {
                trajGroup->addToGroup(seg);
            }
        }

        if (!trajGroup->childItems().isEmpty()) {
            if (!acSignalLine) {
                acSignalLine = new QGraphicsItemGroup();
                plot->getScene()->addItem(acSignalLine);
            }
            acSignalLine->addToGroup(trajGroup);

            const double xScale = plot->getXScale();
            const double yScale = plot->getYScale();
            if (xScale > 0.0 && yScale > 0.0) {
                const double xStart = plot->getXStart();
                const double yStart = plot->getYStart();
                const double xStop  = xStart + static_cast<double>(PLOT_WIDTH)  / xScale;
                const double yStop  = yStart + static_cast<double>(PLOT_HEIGHT) / yScale;

                const double xText = xStart + 0.03 * (xStop - xStart);
                const double yText = yStart + 0.95 * (yStop - yStart);

                const double sx = (xText - xStart) * xScale;
                const double sy = PLOT_HEIGHT - (yText - yStart) * yScale;

                QGraphicsTextItem *t1 = new QGraphicsTextItem("D1 trajectory");
                t1->setDefaultTextColor(penTop.color());
                t1->setPos(sx, sy);
                trajGroup->addToGroup(t1);

                QGraphicsTextItem *t2 = new QGraphicsTextItem("D2 trajectory");
                t2->setDefaultTextColor(penBot.color());
                t2->setPos(sx, sy + 14.0);
                trajGroup->addToGroup(t2);
            }
        } else {
            delete trajGroup;
        }
    }

    // Max swing (brown) and symmetric swing (blue) tick/label helpers along the
    // combined AC load line, mirroring the SE output stage helpers. Labels are
    // placed at negative Ia so that they appear visually below the graph.
    {
        const double slope = gradient; // mA/V, same as AC load line
        const double ia0   = ia;
        const double va0   = inductiveLoad ? vb : vaBias;

        auto ia_line_mA = [&](double va) {
            return ia0 + slope * (va - va0);
        };

        // Left limit: intersection of AC load line with Vg1 = 0 curve at the
        // current screen voltage.
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

        // Right limits along the AC load line: Ia = 0 crossing and optional
        // Pa_max (anode dissipation) limit.
        double vaRight = -1.0;
        if (vaLeft >= 0.0) {
            // Ia = 0 crossing of the AC line.
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

                // Labels at tick positions, placed on a negative Ia row so they
                // appear below the graph.
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

                // Attach helpers to the AC load line group so they move with it.
                if (!acSignalLine) {
                    acSignalLine = new QGraphicsItemGroup();
                    plot->getScene()->addItem(acSignalLine);
                }
                acSignalLine->addToGroup(maxSwingGroup);
            }

            // Mark the maximum-output-power point used for PP_POUT. This is the
            // intersection of the AC load line with the Vg1=0 curve, which is
            // represented here by vaLeft and the corresponding Ia on the line.
            {
                const double iaPmax = ia_line_mA(vaLeft);
                if (std::isfinite(iaPmax) && iaPmax > 0.0) {
                    QGraphicsItemGroup *pmaxGroup = new QGraphicsItemGroup();
                    QPen pPen(QColor::fromRgb(255, 140, 0)); // orange
                    pPen.setWidth(2);

                    const double d = 5.0;
                    if (auto *h = plot->createSegment(vaLeft - d, iaPmax, vaLeft + d, iaPmax, pPen)) {
                        pmaxGroup->addToGroup(h);
                    }
                    if (auto *v = plot->createSegment(vaLeft, iaPmax - d, vaLeft, iaPmax + d, pPen)) {
                        pmaxGroup->addToGroup(v);
                    }

                    if (!acSignalLine) {
                        acSignalLine = new QGraphicsItemGroup();
                        plot->getScene()->addItem(acSignalLine);
                    }
                    acSignalLine->addToGroup(pmaxGroup);
                }
            }

            // Symmetric swing (blue): around the operating point.
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
