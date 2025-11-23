#include "triodecommoncathode.h"

TriodeCommonCathode::TriodeCommonCathode()
{
    // Initialize circuit parameters with sensible defaults
    parameter[TRI_CC_VB] = new Parameter("Supply voltage (V)", 300.0);     // B+ supply
    parameter[TRI_CC_RK] = new Parameter("Cathode resistor (Ω)", 1000.0);   // Cathode resistor
    parameter[TRI_CC_RA] = new Parameter("Anode resistor (Ω)", 100000.0);   // Anode load resistor
    parameter[TRI_CC_RL] = new Parameter("Load impedance (Ω)", 1000000.0);  // Speaker/load impedance

    // Calculated/display parameters
    parameter[TRI_CC_VK] = new Parameter("Bias voltage (V)", 0.0);   // Vk = Ia * Rk
    parameter[TRI_CC_VA] = new Parameter("Anode voltage (V)", 0.0);   // Va = Vb - Ia * Ra
    parameter[TRI_CC_IA] = new Parameter("Anode current (mA)", 0.0);  // Operating current
    parameter[TRI_CC_AR] = new Parameter("Internal resistance (Ω)", 0.0); // Small signal ra
    parameter[TRI_CC_GAIN] = new Parameter("Gain (unbypassed)", 0.0); // Voltage gain
    parameter[TRI_CC_GAIN_B] = new Parameter("Gain (bypassed)", 0.0); // Voltage gain with bypassed cathode
    parameter[TRI_CC_MU] = new Parameter("Mu (unitless)", 0.0); // Small-signal mu at OP
    parameter[TRI_CC_GM] = new Parameter("Transconductance (mA/V)", 0.0); // gm at OP
    parameter[TRI_CC_HEADROOM] = new Parameter("Headroom (Vpk)", 0.0); // Manual headroom override
}


void TriodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Input parameters (editable) - first 4
    for (int i = 0; i < 4; i++) {
        if (parameter[i] && labels[i] && values[i]) {
            if (i == TRI_CC_RA) {
                labels[i]->setText("Anode resistor (kΩ)");
                values[i]->setText(QString::number(parameter[i]->getValue() / 1000.0, 'f', 1));
            } else if (i == TRI_CC_RL) {
                labels[i]->setText("Load impedance (kΩ)");
                values[i]->setText(QString::number(parameter[i]->getValue() / 1000.0, 'f', 1));
            } else {
                labels[i]->setText(parameter[i]->getName());
                values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            }
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
        }
    }

    // Output parameters (read-only) - Vk, Va, Ia, ra, Gain, mu, gm
    for (int i = 4; i < 12; i++) {
        if (parameter[i] && labels[i] && values[i]) {
            QString labelText;
            switch (i) {
                case 4: labelText = "Bias voltage Vk (V):"; break;
                case 5: labelText = "Anode voltage Va (V):"; break;
                case 6: labelText = "Anode current Ia (mA):"; break;
                case 7: labelText = "Internal resistance ra (Ω):"; break;
                case 8: labelText = "Gain:"; break;
                case 9: labelText = ""; break;
                case 10: labelText = "Mu (unitless):"; break;
                case 11: labelText = "gm (mA/V):"; break;
            }
            if (i == 9) {
                // Hide the second gain box in favor of a single mode-dependent Gain
                labels[i]->setVisible(false);
                values[i]->setVisible(false);
            } else {
                labels[i]->setText(labelText);
                if (device1 == nullptr) {
                    values[i]->setText("N/A");
                } else {
                    if (i == 8) {
                        // Single Gain value chosen by K bypass toggle
                        double gain = (sensitivityGainMode == 1) ? parameter[TRI_CC_GAIN_B]->getValue() : parameter[TRI_CC_GAIN]->getValue();
                        values[i]->setText(QString::number(gain, 'f', 1));
                    } else if (i == 7) {
                        // Internal resistance ra: no decimal places
                        values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 0));
                    } else if (i == 11) {
                        // gm (mA/V): two decimal places
                        values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 2));
                    } else {
                        values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
                    }
                }
                // Ensure internal resistance value box is wide enough for large numbers
                if (i == 7) {
                    values[i]->setMinimumWidth(80);
                    values[i]->setMaximumWidth(110);
                }
                labels[i]->setVisible(true);
                values[i]->setVisible(true);
                values[i]->setReadOnly(true);  // Make output parameters read-only
            }
        }
    }

    // Manual headroom input (Vpk) in row 12
    if (labels[12] && values[12]) {
        labels[12]->setText("Headroom (Vpk):");
        if (!device1) {
            values[12]->setText("0.0");
        } else {
            values[12]->setText(QString::number(parameter[TRI_CC_HEADROOM]->getValue(), 'f', 1));
        }

        const double headroomManual = parameter[TRI_CC_HEADROOM]->getValue();
        QString style;
        if (headroomManual > 0.0) {
            // Manual override: bright blue
            style = "color: rgb(0,0,255);";
        } else if (lastSymVpp > 0.0) {
            // Helper-derived symmetric swing
            if (showSymSwing) {
                style = "color: rgb(100,149,237);"; // light blue
            } else {
                style = "color: rgb(165,42,42);";   // brown for max-swing helper (future)
            }
        }
        labels[12]->setStyleSheet(style);
        values[12]->setStyleSheet(style);
        labels[12]->setVisible(true);
        values[12]->setVisible(true);
        values[12]->setReadOnly(false);
    }

    // Input sensitivity (Vpp) as an extra Designer value in row 13
    const int sensIndex = 13;
    if (labels[sensIndex] && values[sensIndex]) {
        labels[sensIndex]->setText("Input sensitivity (Vpp):");
        double vpp_in = 0.0;

        if (device1 != nullptr) {
            const double headroomManual = parameter[TRI_CC_HEADROOM]->getValue();
            double vpp_out = 0.0;

            // Manual headroom takes precedence when > 0
            if (headroomManual > 0.0) {
                vpp_out = 2.0 * headroomManual;
            } else if (showSymSwing && lastSymVpp > 0.0) {
                // Symmetric swing helper when enabled
                vpp_out = lastSymVpp;
            } else if (!showSymSwing && lastMaxVpp > 0.0) {
                // Max swing helper when symmetric swing is disabled
                vpp_out = lastMaxVpp;
            }

            if (vpp_out > 0.0) {
                double gain = (sensitivityGainMode == 1)
                                  ? parameter[TRI_CC_GAIN_B]->getValue()
                                  : parameter[TRI_CC_GAIN]->getValue();
                if (std::isfinite(gain) && std::abs(gain) > 1e-12) {
                    vpp_in = vpp_out / std::abs(gain);
                }
            }
        }

        if (vpp_in > 0.0) {
            values[sensIndex]->setText(QString::number(vpp_in, 'f', 1));

            // Colour to match headroom source: bright blue for manual,
            // light blue for symmetric helper, brown for max swing. Only apply colour when we
            // actually have a sensitivity value.
            QString style;
            const double headroomManual = parameter[TRI_CC_HEADROOM]->getValue();
            if (headroomManual > 0.0) {
                style = "color: rgb(0,0,255);";
            } else if (showSymSwing && lastSymVpp > 0.0) {
                style = "color: rgb(100,149,237);";
            } else if (!showSymSwing && lastMaxVpp > 0.0) {
                style = "color: rgb(165,42,42);";
            }
            values[sensIndex]->setStyleSheet(style);
            labels[sensIndex]->setStyleSheet(style);

            labels[sensIndex]->setVisible(true);
            values[sensIndex]->setVisible(true);
        } else {
            // No valid swing (no manual headroom and symmetric swing off, or
            // no gain): hide the row entirely to avoid stale values.
            values[sensIndex]->setText("");
            values[sensIndex]->setStyleSheet("");
            labels[sensIndex]->setStyleSheet("");
            labels[sensIndex]->setVisible(false);
            values[sensIndex]->setVisible(false);
        }

        values[sensIndex]->setReadOnly(true);
    }

    // Harmonic content at effective headroom (HD2–HD5 and THD)
    const double headroomManual = parameter[TRI_CC_HEADROOM]->getValue();
    double effectiveHeadroomVpk = 0.0;
    if (headroomManual > 0.0) {
        effectiveHeadroomVpk = headroomManual;
    } else if (showSymSwing && lastSymVpp > 0.0) {
        effectiveHeadroomVpk = 0.5 * lastSymVpp; // Vpk from symmetric Vpp
    } else if (!showSymSwing && lastMaxVpp > 0.0) {
        effectiveHeadroomVpk = 0.5 * lastMaxVpp; // Vpk from max Vpp
    }

    auto colourForSource = [&](void) -> QString {
        if (headroomManual > 0.0) {
            return "color: rgb(0,0,255);"; // manual: bright blue
        } else if (showSymSwing && lastSymVpp > 0.0) {
            return "color: rgb(100,149,237);"; // symmetric helper: light blue
        } else if (!showSymSwing && lastMaxVpp > 0.0) {
            return "color: rgb(165,42,42);"; // max swing helper: brown
        }
        return QString();
    };

    const int harmIndex1 = 14; // HD2/HD3
    const int harmIndex2 = 15; // HD4/HD5
    const int thdIndex   = 9;  // reuse hidden row 9 for THD at headroom

    if (device1 && effectiveHeadroomVpk > 0.0 &&
        labels[harmIndex1] && values[harmIndex1] &&
        labels[harmIndex2] && values[harmIndex2] &&
        labels[thdIndex]   && values[thdIndex]) {

        double hd2 = 0.0, hd3 = 0.0, hd4 = 0.0, hd5 = 0.0, thd = 0.0;
        if (simulateHarmonicsTimeDomain(effectiveHeadroomVpk, hd2, hd3, hd4, hd5, thd)) {
            const QString style = colourForSource();

            // Row 14: HD2/HD4 (even harmonics)
            labels[harmIndex1]->setText("HD2/HD4 at headroom (%):");
            values[harmIndex1]->setText(
                QString("%1 / %2")
                    .arg(hd2, 0, 'f', 1)
                    .arg(hd4, 0, 'f', 1));
            labels[harmIndex1]->setStyleSheet(style);
            values[harmIndex1]->setStyleSheet(style);
            // Allow extra space for larger even-harmonic values
            values[harmIndex1]->setMinimumWidth(110);
            values[harmIndex1]->setMaximumWidth(180);
            labels[harmIndex1]->setVisible(true);
            values[harmIndex1]->setVisible(true);
            values[harmIndex1]->setReadOnly(true);

            // Row 15: HD3/HD5 (odd harmonics)
            labels[harmIndex2]->setText("HD3/HD5 at headroom (%):");
            values[harmIndex2]->setText(
                QString("%1 / %2")
                    .arg(hd3, 0, 'f', 1)
                    .arg(hd5, 0, 'f', 1));
            labels[harmIndex2]->setStyleSheet(style);
            values[harmIndex2]->setStyleSheet(style);
            // Allow extra space for larger odd-harmonic values
            values[harmIndex2]->setMinimumWidth(110);
            values[harmIndex2]->setMaximumWidth(180);
            labels[harmIndex2]->setVisible(true);
            values[harmIndex2]->setVisible(true);
            values[harmIndex2]->setReadOnly(true);

            // Row 9: THD
            labels[thdIndex]->setText("THD at headroom (%):");
            values[thdIndex]->setText(QString::number(thd, 'f', 1));
            labels[thdIndex]->setStyleSheet(style);
            values[thdIndex]->setStyleSheet(style);
            labels[thdIndex]->setVisible(true);
            values[thdIndex]->setVisible(true);
            values[thdIndex]->setReadOnly(true);
        } else {
            // Harmonic computation failed; hide rows to avoid stale data.
            for (int i : {harmIndex1, harmIndex2, thdIndex}) {
                if (labels[i] && values[i]) {
                    labels[i]->setVisible(false);
                    values[i]->setVisible(false);
                    labels[i]->setStyleSheet("");
                    values[i]->setStyleSheet("");
                }
            }
        }
    } else {
        // No valid headroom or device; hide harmonic rows.
        for (int i : {harmIndex1, harmIndex2, thdIndex}) {
            if (labels[i] && values[i]) {
                labels[i]->setVisible(false);
                values[i]->setVisible(false);
                labels[i]->setStyleSheet("");
                values[i]->setStyleSheet("");
            }
        }
    }
}

int TriodeCommonCathode::getDeviceType(int index)
{
    if (index == 1) {
        return TRIODE;  // Device 1 is a triode
    } else if (index == 2) {
        return -1;      // No device 2 for this circuit
    }
    return -1;
}

void TriodeCommonCathode::calculateAnodeLoadLine()
{
    double vb = parameter[TRI_CC_VB]->getValue();  // Supply voltage
    double ra = parameter[TRI_CC_RA]->getValue();  // Anode resistor
    double rk = parameter[TRI_CC_RK]->getValue();  // Cathode resistor

    // Maximum anode current = Vb / (Ra + Rk)
    double iaMax = (vb / (ra + rk)) * 1000.0;  // Convert to mA

    // Anode load line: from (0, Ia_max) to (Vb, 0)
    anodeLoadLineData.clear();
    anodeLoadLineData.push_back(QPointF(0.0, iaMax));
    anodeLoadLineData.push_back(QPointF(vb, 0.0));

    qInfo("Designer: Anode line Ia_max=%.2f mA at Va=0, Vb=%.2f V, points=%d", iaMax, vb, anodeLoadLineData.size());
}

void TriodeCommonCathode::calculateCathodeLoadLine()
{
    if (device1 == nullptr) {
        return;  // No device selected
    }

    double rk = parameter[TRI_CC_RK]->getValue();  // Cathode resistor (Ohms)
    double vb = parameter[TRI_CC_VB]->getValue();  // Supply (V)
    double ra = parameter[TRI_CC_RA]->getValue();  // Anode resistor (Ohms)

    // Sweep anode current from 0 to the DC load-line max current
    // For a self-bias cathode: Vg = -Ik*Rk ≈ -Ia*Rk (grid at 0V, cathode positive)
    const int steps = 60;
    cathodeLoadLineData.clear();

    double iaMaxA = 0.05; // fallback 50 mA
    if (std::isfinite(vb) && std::isfinite(ra) && std::isfinite(rk) && (ra + rk) > 0.0) {
        iaMaxA = vb / (ra + rk); // Amps
    }

    for (int i = 0; i <= steps; ++i) {
        double iaA = iaMaxA * (static_cast<double>(i) / steps); // Amps
        double vg = -iaA * rk; // Volts, strictly non-positive

        // Solve for anode voltage that gives this current at this grid bias
        // Model expects Ia in mA to match anodeCurrent units
        double va = device1->anodeVoltage(iaA * 1000.0, vg);

        // Keep points within non-negative plotting domain (Va>=0, Ia>=0)
        if (std::isfinite(va) && va >= 0.0 && iaA >= 0.0) {
            cathodeLoadLineData.push_back(QPointF(va, iaA * 1000.0)); // store current in mA for plotting
        }
    }

    // Debug: log cathode line stats
    if (!cathodeLoadLineData.isEmpty()) {
        double minVa = cathodeLoadLineData.first().x();
        double maxVa = cathodeLoadLineData.first().x();
        double minIa = cathodeLoadLineData.first().y();
        double maxIa = cathodeLoadLineData.first().y();
        for (const auto &p : cathodeLoadLineData) {
            if (p.x() < minVa) minVa = p.x();
            if (p.x() > maxVa) maxVa = p.x();
            if (p.y() < minIa) minIa = p.y();
            if (p.y() > maxIa) maxIa = p.y();
        }
        qInfo("Designer: Cathode line points=%d Va[%.2f..%.2f] Ia(mA)[%.2f..%.2f]", cathodeLoadLineData.size(), minVa, maxVa, minIa, maxIa);
    } else {
        qInfo("Designer: Cathode line has 0 points (check device selection and parameter values)");
    }
}

QPointF TriodeCommonCathode::findOperatingPoint()
{
    // Find intersection between anode and cathode load lines
    // Use linear interpolation to find where the two lines cross

    if (anodeLoadLineData.size() < 2 || cathodeLoadLineData.empty()) {
        // Not enough data for intersection calculation
        qInfo("Designer: OP: insufficient data (anode pts=%d, cathode pts=%d) - using simple estimate",
              anodeLoadLineData.size(), cathodeLoadLineData.size());
        return findOperatingPointSimple();
    }

    // For each segment of the anode load line, check for intersection with cathode load line
    for (int i = 0; i < anodeLoadLineData.size() - 1; i++) {
        QPointF anodeStart = anodeLoadLineData[i];
        QPointF anodeEnd = anodeLoadLineData[i + 1];

        // Check intersection with each cathode load line segment
        for (int j = 0; j < cathodeLoadLineData.size() - 1; j++) {
            QPointF cathodeStart = cathodeLoadLineData[j];
            QPointF cathodeEnd = cathodeLoadLineData[j + 1];

            QPointF intersection = findLineIntersection(anodeStart, anodeEnd, cathodeStart, cathodeEnd);
            if (intersection.x() >= 0 && intersection.y() >= 0) {
                // Reject degenerate intersections that sit effectively at the origin (Va too close to 0V).
                // In these cases, fall back to the simple estimate rather than using an unusable bias point.
                if (intersection.x() < 1.0) {
                    qInfo("Designer: OP: intersection at Va=%.2f V is too close to 0V - ignoring (seg a%d/c%d)",
                          intersection.x(), i, j);
                    continue;
                }

                // Valid intersection point found
                qInfo("Designer: OP: found intersection at Va=%.2f V, Ia=%.2f mA (seg a%d/c%d)",
                      intersection.x(), intersection.y(), i, j);
                return intersection;
            }
        }
    }

    // If no intersection found, fall back to simple estimation
    qInfo("Designer: OP: no intersection found - using simple estimate");
    return findOperatingPointSimple();
}

QPointF TriodeCommonCathode::findLineIntersection(QPointF line1Start, QPointF line1End,
                                                 QPointF line2Start, QPointF line2End)
{
    // Line intersection algorithm using parametric equations
    double x1 = line1Start.x(), y1 = line1Start.y();
    double x2 = line1End.x(), y2 = line1End.y();
    double x3 = line2Start.x(), y3 = line2Start.y();
    double x4 = line2End.x(), y4 = line2End.y();

    // Calculate denominators
    double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    if (abs(denom) < 1e-10) {
        // Lines are parallel or coincident
        return QPointF(-1, -1);  // Invalid intersection
    }

    // Calculate intersection point
    double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    // Check if intersection is within line segments
    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        double ix = x1 + t * (x2 - x1);
        double iy = y1 + t * (y2 - y1);
        return QPointF(ix, iy);
    }

    return QPointF(-1, -1);  // No valid intersection
}

QPointF TriodeCommonCathode::findOperatingPointSimple()
{
    // Simple estimation fallback
    double vb = parameter[TRI_CC_VB]->getValue();
    double ra = parameter[TRI_CC_RA]->getValue();
    double rk = parameter[TRI_CC_RK]->getValue();

    // Estimate: Va ≈ Vb/2, Ia ≈ Vb/(2*(ra+rk))
    double va = vb / 2.0;
    double ia = (vb / (2.0 * (ra + rk))) * 1000.0;

    qInfo("Designer: OP simple estimate Va=%.2f V, Ia=%.2f mA", va, ia);
    return QPointF(va, ia);
}

void TriodeCommonCathode::calculateOperatingPoint()
{
    if (device1 == nullptr) {
        return;  // No device selected
    }

    QPointF op = findOperatingPoint();

    double va = op.x();
    double ia = op.y() / 1000.0;  // Convert to amps for calculations

    // Calculate bias voltages and currents
    double vb = parameter[TRI_CC_VB]->getValue();
    double ra = parameter[TRI_CC_RA]->getValue();
    double rk = parameter[TRI_CC_RK]->getValue();

    double vk = ia * rk;  // Cathode voltage = Ia * Rk
    double vg = 0.0;      // Grid is at ground for now

    // Update calculated parameters
    parameter[TRI_CC_VK]->setValue(vk);
    parameter[TRI_CC_VA]->setValue(va);
    parameter[TRI_CC_IA]->setValue(ia * 1000.0);  // Convert back to mA

    // Calculate small-signal parameters from device at OP using finite differences
    double mu = 0.0;
    double ra_internal = 0.0;  // Ohms
    double gm_mA_per_V = 0.0;  // mA/V

    if (device1 && std::isfinite(va) && std::isfinite(ia)) {
        // Operating point quantities
        const double ia_mA = ia * 1000.0; // mA
        const double vg0 = -ia * rk;      // V (self-bias, grid at 0V)

        // Steps for numerical derivatives
        const double dIa_mA = std::max(0.1, std::abs(ia_mA) * 0.01);   // at least 0.1 mA
        const double dVg = std::max(0.01, std::abs(vg0) * 0.02);       // at least 10 mV

        // Clamp Vg within device limits (non-positive)
        const double vgMin = -device1->getVg1Max();
        const double vgPlus = std::clamp(vg0 + dVg, vgMin, 0.0);
        const double vgMinus = std::clamp(vg0 - dVg, vgMin, 0.0);

        // ra = dVa/dIa (hold Vg constant at vg0). Units: V / A → Ohms
        double va_plusIa = device1->anodeVoltage(ia_mA + dIa_mA, vg0);
        double va_minusIa = device1->anodeVoltage(ia_mA - dIa_mA, vg0);
        if (std::isfinite(va_plusIa) && std::isfinite(va_minusIa) && dIa_mA > 0.0) {
            double dVa_dIa_V_per_mA = (va_plusIa - va_minusIa) / (2.0 * dIa_mA);
            ra_internal = dVa_dIa_V_per_mA * 1000.0; // convert V/mA to Ohms
        }

        // mu = - dVa/dVg (hold Ia constant at ia_mA). Unitless
        double va_plusVg = device1->anodeVoltage(ia_mA, vgPlus);
        double va_minusVg = device1->anodeVoltage(ia_mA, vgMinus);
        if (std::isfinite(va_plusVg) && std::isfinite(va_minusVg) && (vgPlus - vgMinus) != 0.0) {
            double dVa_dVg = (va_plusVg - va_minusVg) / (vgPlus - vgMinus);
            mu = -dVa_dVg;
        }

        // gm = dIa/dVg at constant Va (use model's anodeCurrent). Units mA/V directly
        const double va_hold = va; // hold at OP anode voltage
        double ia_plusVg_mA = device1->anodeCurrent(va_hold, vgPlus);
        double ia_minusVg_mA = device1->anodeCurrent(va_hold, vgMinus);
        if (std::isfinite(ia_plusVg_mA) && std::isfinite(ia_minusVg_mA) && (vgPlus - vgMinus) != 0.0) {
            gm_mA_per_V = (ia_plusVg_mA - ia_minusVg_mA) / (vgPlus - vgMinus); // mA/V
        }
    }

    // Fallbacks if derivatives failed
    if (!std::isfinite(mu) || mu <= 0.0) mu = 100.0;
    if (!std::isfinite(ra_internal) || ra_internal <= 0.0) ra_internal = 1000.0;

    parameter[TRI_CC_AR]->setValue(ra_internal);
    parameter[TRI_CC_MU]->setValue(mu);
    parameter[TRI_CC_GM]->setValue(gm_mA_per_V);

    // Calculate gains using device-derived mu and ra
    double rl = parameter[TRI_CC_RL]->getValue();
    // Effective AC anode load is the parallel of external anode resistor and external load
    double r_ac = (ra > 0.0 && rl > 0.0) ? (ra * rl) / (ra + rl) : (ra > 0.0 ? ra : rl);
    double gain_unbypassed = mu * r_ac / (r_ac + ra_internal + (mu + 1.0) * rk);
    double gain_bypassed = mu * r_ac / (r_ac + ra_internal);

    if (!std::isfinite(gain_unbypassed)) gain_unbypassed = 0.0;
    if (!std::isfinite(gain_bypassed)) gain_bypassed = 0.0;

    parameter[TRI_CC_GAIN]->setValue(gain_unbypassed);
    parameter[TRI_CC_GAIN_B]->setValue(gain_bypassed);
}

void TriodeCommonCathode::update(int index)
{
    // When parameters change, recalculate the circuit
    calculateAnodeLoadLine();
    calculateCathodeLoadLine();
    calculateOperatingPoint();
    calculateGains();

    // TODO: Trigger plot update when UI is connected
}

bool TriodeCommonCathode::computeHeadroomHarmonicCurrents(double headroomVpk,
                                                          double &Ia,
                                                          double &Ib,
                                                          double &Ic,
                                                          double &Id,
                                                          double &Ie) const
{
    Ia = Ib = Ic = Id = Ie = 0.0;

    if (!device1 || headroomVpk <= 0.0) {
        return false;
    }

    const double vb = parameter[TRI_CC_VB]->getValue();
    const double vaOp = parameter[TRI_CC_VA]->getValue();
    const double iaOp_mA = parameter[TRI_CC_IA]->getValue();
    const double rk = parameter[TRI_CC_RK]->getValue();

    if (!(vb > 0.0) || !(iaOp_mA > 0.0) || !(rk >= 0.0)) {
        return false;
    }

    // Clamp headroom to a sensible fraction of the available B+ range so the
    // swing does not exceed physical limits.
    const double maxHeadroom = 0.9 * std::max(1.0, std::min(vb, device1->getVaMax()));
    const double vpk = std::min(std::max(0.0, headroomVpk), maxHeadroom);
    if (vpk <= 0.0) {
        return false;
    }

    const double iaOp_A = iaOp_mA / 1000.0;
    const double vg0 = -iaOp_A * rk; // grid-to-cathode bias (self-bias, grid at 0V)

    auto sampleCurrent = [&](double va) -> double {
        // Clamp Va into [0, Va_max]
        const double vaClamped = std::clamp(va, 0.0, device1->getVaMax());
        const double ia_mA = device1->anodeCurrent(vaClamped, vg0);
        if (!std::isfinite(ia_mA) || ia_mA < 0.0) {
            return 0.0;
        }
        return ia_mA / 1000.0; // convert to amps
    };

    const double va0 = vaOp + vpk;        // positive peak
    const double va1 = vaOp + 0.5 * vpk;  // positive mid
    const double va2 = vaOp;              // bias point
    const double va3 = vaOp - 0.5 * vpk;  // negative mid
    const double va4 = vaOp - vpk;        // negative peak

    Ia = sampleCurrent(va0);
    Ib = sampleCurrent(va1);
    Ic = sampleCurrent(va2);
    Id = sampleCurrent(va3);
    Ie = sampleCurrent(va4);

    if (!(Ia > 0.0) || !(Ic > 0.0)) {
        return false;
    }

    return true;
}

bool TriodeCommonCathode::simulateHarmonicsTimeDomain(double headroomVpk,
                                                       double &hd2,
                                                       double &hd3,
                                                       double &hd4,
                                                       double &hd5,
                                                       double &thd) const
{
    hd2 = hd3 = hd4 = hd5 = thd = 0.0;

    if (!device1 || headroomVpk <= 0.0) {
        return false;
    }

    double Ia = 0.0;
    double Ib = 0.0;
    double Ic = 0.0;
    double Id = 0.0;
    double Ie = 0.0;
    if (!computeHeadroomHarmonicCurrents(headroomVpk,
                                         Ia,
                                         Ib,
                                         Ic,
                                         Id,
                                         Ie)) {
        return false;
    }

    // Arrange the five samples (in amps) as one period of a periodic waveform.
    double samples[5];
    samples[0] = Ia;
    samples[1] = Ib;
    samples[2] = Ic;
    samples[3] = Id;
    samples[4] = Ie;

    const int sampleCount = 512;
    const double twoPi = 6.28318530717958647692; // 2 * pi

    double a[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double b[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    for (int k = 0; k < sampleCount; ++k) {
        const double phase = twoPi * static_cast<double>(k) / static_cast<double>(sampleCount);
        const double u = phase / twoPi; // normalised phase in [0, 1)

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

        const double window = 0.5 * (1.0 - std::cos(twoPi * static_cast<double>(k) /
                                                   static_cast<double>(sampleCount - 1)));
        const double v = ip * window;

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

void TriodeCommonCathode::computeTimeDomainHarmonicScan(QVector<double> &headroomVals,
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

    if (!device1) {
        return;
    }

    const double vb  = parameter[TRI_CC_VB]->getValue();
    const double va  = parameter[TRI_CC_VA]->getValue();

    if (!(vb > 0.0) || !(va > 0.0)) {
        return;
    }

    const double maxHeadroom = 0.9 * std::max(1.0, std::min(vb, device1->getVaMax()));
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
        if (simulateHarmonicsTimeDomain(head, hd2, hd3, hd4, hd5, thd)) {
            headroomVals.push_back(head);
            hd2Vals.push_back(hd2);
            hd3Vals.push_back(hd3);
            hd4Vals.push_back(hd4);
            thdVals.push_back(thd);
        }
    }
}

void TriodeCommonCathode::plot(Plot *plot)
{
    qInfo("Designer: TriodeCC plot() start. Device=%s, Vb=%.2f, Ra=%.0f, Rk=%.0f, RL=%.0f",
          device1 ? device1->getName().toStdString().c_str() : "<none>",
          parameter[TRI_CC_VB]->getValue(),
          parameter[TRI_CC_RA]->getValue(),
          parameter[TRI_CC_RK]->getValue(),
          parameter[TRI_CC_RL]->getValue());
    // Clear any existing circuit overlays (Designer-only). Guard against cases
    // where the shared Plot scene has been cleared externally (e.g., via
    // Plot::setAxes in selectStdDevice) by only removing items that still
    // belong to this scene, then dropping our pointers. We intentionally do
    // NOT delete the groups here; QGraphicsScene owns and destroys them when
    // they are cleared from the scene.
    lastSymVpp = 0.0;

    if (anodeLoadLine != nullptr) {
        if (anodeLoadLine->scene() == plot->getScene()) {
            plot->getScene()->removeItem(anodeLoadLine);
        }
        anodeLoadLine = nullptr;
    }

    if (cathodeLoadLine != nullptr) {
        if (cathodeLoadLine->scene() == plot->getScene()) {
            plot->getScene()->removeItem(cathodeLoadLine);
        }
        cathodeLoadLine = nullptr;
    }

    if (acSignalLine != nullptr) {
        if (acSignalLine->scene() == plot->getScene()) {
            plot->getScene()->removeItem(acSignalLine);
        }
        acSignalLine = nullptr;
    }

    if (opMarker != nullptr) {
        if (opMarker->scene() == plot->getScene()) {
            plot->getScene()->removeItem(opMarker);
        }
        opMarker = nullptr;
    }

    if (swingGroup != nullptr) {
        if (swingGroup->scene() == plot->getScene()) {
            plot->getScene()->removeItem(swingGroup);
        }
        swingGroup = nullptr;
    }

    if (symSwingGroup != nullptr) {
        if (symSwingGroup->scene() == plot->getScene()) {
            plot->getScene()->removeItem(symSwingGroup);
        }
        symSwingGroup = nullptr;
    }

    if (sensitivityGroup != nullptr) {
        if (sensitivityGroup->scene() == plot->getScene()) {
            plot->getScene()->removeItem(sensitivityGroup);
        }
        sensitivityGroup = nullptr;
    }

    if (paLimitGroup != nullptr) {
        if (paLimitGroup->scene() == plot->getScene()) {
            plot->getScene()->removeItem(paLimitGroup);
        }
        paLimitGroup = nullptr;
    }

    // Check if we have a device selected
    if (device1 == nullptr) {
        // No device selected - can't plot load lines
        return;
    }

    // Generate fresh load line data
    calculateAnodeLoadLine();
    calculateCathodeLoadLine();
    QPointF op = findOperatingPoint();

    qInfo("Designer: TriodeCC OP estimate Va=%.2f V, Ia=%.2f mA", op.x(), op.y());

    // Set axes strictly to device limits to match model plot size
    double vb = parameter[TRI_CC_VB]->getValue();
    double ra = parameter[TRI_CC_RA]->getValue();
    double rk = parameter[TRI_CC_RK]->getValue();

    double iaDevice = (device1 ? device1->getIaMax() : 5.0);    // mA
    double vaDevice = (device1 ? device1->getVaMax() : vb);     // V

    double yStop = iaDevice;
    double xStop = vaDevice;
    double yMajor = std::max(0.5, yStop / 10.0);
    double xMajor = std::max(5.0, xStop / 10.0);

    // Preserve existing axes if other plots are already drawn
    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, xStop, xMajor, 0.0, yStop, yMajor);
    }

    // Plot anode load line (green)
    if (anodeLoadLineData.size() >= 2) {
        anodeLoadLine = new QGraphicsItemGroup();

        QPen anodePen;
        anodePen.setColor(QColor::fromRgb(0, 128, 0));  // Green for anode load line
        anodePen.setWidth(2);

        for (int i = 0; i < anodeLoadLineData.size() - 1; i++) {
            QPointF start = anodeLoadLineData[i];
            QPointF end = anodeLoadLineData[i + 1];
            // Clamp within device limits
            start.setX(std::clamp(start.x(), 0.0, xStop));
            end.setX(std::clamp(end.x(), 0.0, xStop));
            start.setY(std::clamp(start.y(), 0.0, yStop));
            end.setY(std::clamp(end.y(), 0.0, yStop));

            QGraphicsLineItem *segment = plot->createSegment(start.x(), start.y(), end.x(), end.y(), anodePen);
            if (segment) {
                anodeLoadLine->addToGroup(segment);
            }
        }

        plot->getScene()->addItem(anodeLoadLine);
    } else {
        qInfo("Designer: anode line has insufficient points: %d", anodeLoadLineData.size());
    }

    // Plot cathode load line (purple) - only if we have valid data
    if (cathodeLoadLineData.size() >= 2) {
        qInfo("Designer: TriodeCC plot() drawing cathode load line, points=%d", cathodeLoadLineData.size());
        cathodeLoadLine = new QGraphicsItemGroup();

        QPen cathodePen;
        cathodePen.setColor(QColor::fromRgb(128, 0, 128));  // Blue for cathode load line
        cathodePen.setWidth(2);

        // Left/right margins to avoid hugging the plot frame visually
        const double leftClipX = std::max(5.0, xStop * 0.03);  // ~3% of width or 5V
        const double rightClipX = std::max(leftClipX + 1.0, xStop - std::max(5.0, xStop * 0.03));

        for (int i = 0; i < cathodeLoadLineData.size() - 1; i++) {
            QPointF s0 = cathodeLoadLineData[i];
            QPointF e0 = cathodeLoadLineData[i + 1];
            // Skip segments entirely left of left margin or entirely right of right margin
            if ((s0.x() < leftClipX && e0.x() < leftClipX) || (s0.x() > rightClipX && e0.x() > rightClipX)) continue;

            // If the segment crosses the margin from left to right, interpolate start at leftClipX
            QPointF start = s0;
            QPointF end = e0;
            if (start.x() < leftClipX && end.x() > leftClipX && (end.x() - start.x()) != 0.0) {
                double t = (leftClipX - start.x()) / (end.x() - start.x());
                double yAtClip = start.y() + t * (end.y() - start.y());
                start.setX(leftClipX);
                start.setY(yAtClip);
            }
            // If the segment crosses the right margin from left to right, interpolate end at rightClipX
            if (start.x() < rightClipX && end.x() > rightClipX && (end.x() - start.x()) != 0.0) {
                double t = (rightClipX - start.x()) / (end.x() - start.x());
                double yAtClip = start.y() + t * (end.y() - start.y());
                end.setX(rightClipX);
                end.setY(yAtClip);
            }

            // Clamp within device limits
            start.setX(std::clamp(start.x(), 0.0, xStop));
            end.setX(std::clamp(end.x(), 0.0, xStop));
            start.setY(std::clamp(start.y(), 0.0, yStop));
            end.setY(std::clamp(end.y(), 0.0, yStop));

            // Ensure we still have a drawable span
            if (end.x() <= leftClipX || start.x() >= rightClipX) continue;

            QGraphicsLineItem *segment = plot->createSegment(start.x(), start.y(), end.x(), end.y(), cathodePen);
            if (segment) {
                cathodeLoadLine->addToGroup(segment);
            }
        }

        plot->getScene()->addItem(cathodeLoadLine);
    } else {
        qInfo("Designer: cathode line has insufficient points: %d", cathodeLoadLineData.size());
    }

    // Mark operating point (red crosshair)
    if (op.x() >= 0.0 && op.y() >= 0.0) {
        qInfo("Designer: TriodeCC plot() drawing OP marker at Va=%.2f, Ia=%.2f", op.x(), op.y());
        QPen opPen;
        opPen.setColor(QColor::fromRgb(255, 0, 0));  // Red for operating point
        opPen.setWidth(2);

        const double d = 5.0; // small span in data units
        opMarker = new QGraphicsItemGroup();
        QGraphicsLineItem *h = plot->createSegment(op.x() - d, op.y(), op.x() + d, op.y(), opPen);
        QGraphicsLineItem *v = plot->createSegment(op.x(), op.y() - d, op.x(), op.y() + d, opPen);
        if (h) opMarker->addToGroup(h);
        if (v) opMarker->addToGroup(v);
        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker; opMarker = nullptr;
        }
    }

    qInfo("Designer: TriodeCC plot() entering AC load line block");
    // Plot small-signal AC load line (yellow) around operating point
    if (op.x() >= 0 && op.y() >= 0) {
        double ra = parameter[TRI_CC_RA]->getValue();
        double rl = parameter[TRI_CC_RL]->getValue();
        double rpar = (ra > 0.0 && rl > 0.0) ? (ra * rl) / (ra + rl) : ra > 0.0 ? ra : rl;
        if (rpar > 0.0 && std::isfinite(rpar)) {
            // slope in mA/V is -1000 / R_parallel
            double slope = -1000.0 / rpar;

            QPen acPen;
            acPen.setColor(QColor::fromRgb(255, 215, 0));  // Yellow for AC line
            acPen.setWidth(2);

            // Span full graph width from 0 to xStop, passing through OP
            double x1 = 0.0;
            double x2 = xStop;
            double y1 = op.y() + slope * (x1 - op.x());
            double y2 = op.y() + slope * (x2 - op.x());

            // Create a group so it can be cleared on next replot
            acSignalLine = new QGraphicsItemGroup();
            QGraphicsItem *segment = plot->createSegment(x1, y1, x2, y2, acPen);
            if (segment) acSignalLine->addToGroup(segment);
            if (!acSignalLine->childItems().isEmpty()) {
                plot->getScene()->addItem(acSignalLine);
            } else {
                delete acSignalLine; acSignalLine = nullptr;
            }

            // Remove previous swing annotations
            if (swingGroup) {
                plot->getScene()->removeItem(swingGroup);
                delete swingGroup;
                swingGroup = nullptr;
            }

            // Compute intersection with Vg=0 model curve (prefer left of OP): scan from OP.x() downwards
            double vaMax = xStop;
            double vaCut = -1.0;
            double yAtCut = 0.0;
            auto f = [&](double va){
                // anodeCurrent returns mA; slope is in mA/V; op.y() is in mA
                double ia_curve_mA = device1->anodeCurrent(va, 0.0);
                double ia_line_mA = op.y() + slope * (va - op.x());
                return ia_curve_mA - ia_line_mA;
            };
            const int samples = 600;
            double lastVa = op.x();
            lastVa = std::clamp(lastVa, 0.0, vaMax);
            double lastF = f(lastVa);
            for (int i = 1; i <= samples; ++i) {
                double va = op.x() * (1.0 - static_cast<double>(i) / samples); // decreasing from OP to 0
                va = std::max(va, 0.0);
                double curF = f(va);
                if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                    double denom = (curF - lastF);
                    double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                    t = std::clamp(t, 0.0, 1.0);
                    vaCut = lastVa + t * (va - lastVa);
                    yAtCut = op.y() + slope * (vaCut - op.x());
                    break;
                }
                lastVa = va;
                lastF = curF;
            }
            // If no sign change found, fall back to closest match away from boundary
            if (vaCut < 0.0) {
                double bestVa = -1.0;
                double bestAbs = std::numeric_limits<double>::infinity();
                for (int i = 0; i <= samples; ++i) {
                    double va = op.x() * (static_cast<double>(i) / samples);
                    double curF = f(va);
                    double absF = std::abs(curF);
                    if (absF < bestAbs && va > 1.0 && va < vaMax - 1.0) {
                        bestAbs = absF;
                        bestVa = va;
                    }
                }
                if (bestVa >= 0.0) {
                    vaCut = bestVa;
                    yAtCut = op.y() + slope * (vaCut - op.x());
                }
            }

            // Intersection with Ia = 0 (x-axis)
            double vaZero = op.x() - op.y() / slope;
            vaZero = std::clamp(vaZero, 0.0, xStop);

            // Draw annotations if valid (vertical cutoff line + labels)
            swingGroup = new QGraphicsItemGroup();
            QPen swingPen(QColor::fromRgb(165, 42, 42)); // brown cutoff line
            swingPen.setWidth(2);
            if (vaCut >= 0.0) {
                QGraphicsLineItem *vline = plot->createSegment(vaCut, 0.0, vaCut, yAtCut, swingPen);
                if (vline) swingGroup->addToGroup(vline);
                QGraphicsTextItem *labelCut = plot->createLabel(vaCut, -yMajor * 1.8, vaCut, QColor::fromRgb(165,42,42));
                if (labelCut) {
                    // Center horizontally at vaCut
                    QPointF p = labelCut->pos();
                    double w = labelCut->boundingRect().width();
                    labelCut->setPos(p.x() - 5.0 - w / 2.0, p.y());
                    swingGroup->addToGroup(labelCut);
                }
            }
            QGraphicsTextItem *labelZero = plot->createLabel(vaZero, -yMajor * 1.8, vaZero, QColor::fromRgb(165,42,42));
            if (labelZero) {
                QPointF pz = labelZero->pos();
                double wz = labelZero->boundingRect().width();
                labelZero->setPos(pz.x() - 5.0 - wz / 2.0, pz.y());
                swingGroup->addToGroup(labelZero);
            }
            if (vaCut >= 0.0) {
                double swing = std::abs(vaZero - vaCut);
                double mid = 0.5 * (vaZero + vaCut);
                QGraphicsTextItem *labelSwing = plot->createLabel(mid, -yMajor * 1.8, swing, QColor::fromRgb(165,42,42));
                if (labelSwing) {
                    QPointF ps = labelSwing->pos();
                    double ws = labelSwing->boundingRect().width();
                    labelSwing->setPos(ps.x() - 5.0 - ws / 2.0, ps.y());
                    swingGroup->addToGroup(labelSwing);
                }
            }
            if (!swingGroup->childItems().isEmpty()) {
                plot->getScene()->addItem(swingGroup);
            } else {
                delete swingGroup; swingGroup = nullptr;
            }
        }
    }

    qInfo("Designer: TriodeCC plot() entering Pa limit block");
    // Draw maximum dissipation limit (Pa = constant) as dashed line
    if (device1) {
        const double paMaxW = device1->getPaMax(); // Watts
        if (paMaxW > 0.0) {
            paLimitGroup = new QGraphicsItemGroup();
            QPen paPen(QColor::fromRgb(180, 0, 0));
            paPen.setStyle(Qt::DashLine);
            paPen.setWidth(2);

            // Start where the Pa curve enters the visible y-range to avoid hugging the top-left edge
            const double xEnter = std::max(1e-6, std::min(xStop, (yStop > 0.0 ? (1000.0 * paMaxW / yStop) : xStop)));
            const int segs = 60;
            double prevX = xEnter;
            double prevY = std::min(yStop, 1000.0 * paMaxW / prevX);
            for (int i = 1; i <= segs; ++i) {
                double t = static_cast<double>(i) / segs;
                double x = xEnter + (xStop - xEnter) * t;
                double y = (x > 0.0) ? std::min(yStop, 1000.0 * paMaxW / x) : yStop;
                QGraphicsLineItem *seg = plot->createSegment(prevX, prevY, x, y, paPen);
                if (seg) paLimitGroup->addToGroup(seg);
                prevX = x;
                prevY = y;
            }
            if (!paLimitGroup->childItems().isEmpty()) {
                plot->getScene()->addItem(paLimitGroup);
            } else {
                delete paLimitGroup; paLimitGroup = nullptr;
            }
        }
    }

    qInfo("Designer: TriodeCC plot() entering symmetric swing block");
    // Symmetrical and max swing helpers (Vpp), conditional
    if (op.x() >= 0.0 && op.y() >= 0.0 && device1) {
        double ra = parameter[TRI_CC_RA]->getValue();
        double rl = parameter[TRI_CC_RL]->getValue();
        double rpar = (ra > 0.0 && rl > 0.0) ? (ra * rl) / (ra + rl) : ra > 0.0 ? ra : rl;
        if (rpar > 0.0 && std::isfinite(rpar)) {
            const double slope = -1000.0 / rpar; // mA/V
            // Compute left cutoff again
            auto f_vg0 = [&](double va){
                double ia_curve_mA = device1->anodeCurrent(va, 0.0);
                double ia_line_mA = op.y() + slope * (va - op.x());
                return ia_curve_mA - ia_line_mA;
            };
            double vaCut = -1.0; double yAtCut = 0.0;
            {
                const int samples = 400;
                double lastVa = std::clamp(op.x(), 0.0, xStop);
                double lastF = f_vg0(lastVa);
                for (int i = 1; i <= samples; ++i) {
                    double va = op.x() * (1.0 - static_cast<double>(i) / samples);
                    va = std::max(va, 0.0);
                    double curF = f_vg0(va);
                    if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                        double denom = (curF - lastF);
                        double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                        t = std::clamp(t, 0.0, 1.0);
                        vaCut = lastVa + t * (va - lastVa);
                        yAtCut = op.y() + slope * (vaCut - op.x());
                        break;
                    }
                    lastVa = va; lastF = curF;
                }
            }
            // Right Ia=0 intercept
            double vaZero = op.x() - op.y() / slope;
            vaZero = std::clamp(vaZero, 0.0, xStop);
            // Right Pa limit
            double vaPa = xStop + 1.0;
            if (device1->getPaMax() > 0.0) {
                auto g_pa = [&](double va){
                    if (va <= 0.0) return 1e9;
                    double ia_line_mA = op.y() + slope * (va - op.x());
                    double ia_pa_mA = 1000.0 * device1->getPaMax() / va;
                    return ia_line_mA - ia_pa_mA;
                };
                const int samples = 400;
                double lastVa = std::max(op.x(), 1e-3);
                double lastF = g_pa(lastVa);
                for (int i = 1; i <= samples; ++i) {
                    double va = op.x() + (xStop - op.x()) * (static_cast<double>(i) / samples);
                    double curF = g_pa(va);
                    if ((lastF <= 0.0 && curF >= 0.0) || (lastF >= 0.0 && curF <= 0.0)) {
                        double denom = (curF - lastF);
                        double t = (std::abs(denom) > 1e-12) ? (-lastF / denom) : 0.5;
                        t = std::clamp(t, 0.0, 1.0);
                        vaPa = lastVa + t * (va - lastVa);
                        break;
                    }
                    lastVa = va; lastF = curF;
                }
            }
            const double vaRight = std::min(vaZero, vaPa);

            // Cache maximum peak-to-peak swing along the AC line between
            // the left Vg=0 cutoff and the rightmost valid point (either
            // Ia=0 or Pa limit). This is used when Max Sym Swing is off.
            if (vaCut >= 0.0 && vaRight > vaCut) {
                lastMaxVpp = std::max(0.0, vaRight - vaCut);
            } else {
                lastMaxVpp = 0.0;
            }

            if (showSymSwing && vaCut >= 0.0 && vaRight > op.x() && vaCut < op.x()) {
                const double vpk = std::min(op.x() - vaCut, vaRight - op.x());
                const double leftX = op.x() - vpk;
                const double rightX = op.x() + vpk;
                symSwingGroup = new QGraphicsItemGroup();
                QPen tickPenLeft(QColor::fromRgb(100, 149, 237));  // light blue
                tickPenLeft.setWidth(2);
                QPen tickPenRight(QColor::fromRgb(100, 149, 237)); // light blue
                tickPenRight.setWidth(2);
                // vertical ticks to x-axis
                if (leftX >= 0.0) {
                    if (auto *lt = plot->createSegment(leftX, 0.0, leftX, op.y() + slope * (leftX - op.x()), tickPenLeft)) symSwingGroup->addToGroup(lt);
                }
                if (rightX <= xStop) {
                    if (auto *rt = plot->createSegment(rightX, 0.0, rightX, op.y() + slope * (rightX - op.x()), tickPenRight)) symSwingGroup->addToGroup(rt);
                }
                // Labels at the tick positions (light blue), same row as center label
                {
                    const QColor tickLabelColor = QColor::fromRgb(100, 149, 237);
                    if (leftX >= 0.0) {
                        QGraphicsTextItem *lLbl = plot->createLabel(leftX, -yMajor * 2.4, leftX, tickLabelColor);
                        if (lLbl) {
                            QPointF pl = lLbl->pos();
                            double wl = lLbl->boundingRect().width();
                            lLbl->setPos(pl.x() - 5.0 - wl / 2.0, pl.y());
                            symSwingGroup->addToGroup(lLbl);
                        }
                    }
                    if (rightX <= xStop) {
                        QGraphicsTextItem *rLbl = plot->createLabel(rightX, -yMajor * 2.4, rightX, tickLabelColor);
                        if (rLbl) {
                            QPointF pr = rLbl->pos();
                            double wr = rLbl->boundingRect().width();
                            rLbl->setPos(pr.x() - 5.0 - wr / 2.0, pr.y());
                            symSwingGroup->addToGroup(rLbl);
                        }
                    }
                }
                // centered Vpp label (grey, one row below x-axis labels)
                const double vpp = 2.0 * vpk;
                lastSymVpp = vpp;
                QGraphicsTextItem *lbl = plot->createLabel(op.x(), -yMajor * 2.4, vpp, QColor::fromRgb(100,149,237));
                if (lbl) {
                    QPointF p = lbl->pos();
                    double w = lbl->boundingRect().width();
                    lbl->setPos(p.x() - 5.0 - w / 2.0, p.y());
                    symSwingGroup->addToGroup(lbl);
                }
                if (!symSwingGroup->childItems().isEmpty()) plot->getScene()->addItem(symSwingGroup);
                else { delete symSwingGroup; symSwingGroup = nullptr; }
            }

            // Always update lastSymVpp so Designer box can use it, even if we don't draw an overlay
            {
                double vpk;
                if (vaCut >= 0.0 && vaRight > op.x() && vaCut < op.x()) {
                    vpk = std::min(op.x() - vaCut, vaRight - op.x());
                } else {
                    vpk = std::max(0.0, std::min(op.x(), vaRight - op.x()));
                }
                lastSymVpp = 2.0 * vpk;
            }
        }
    }
}

QTreeWidgetItem *TriodeCommonCathode::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, "Triode Common Cathode");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return item;
}

void TriodeCommonCathode::calculateGains()
{
    // This method calculates the voltage gains for the circuit
    // Gains are already calculated in calculateOperatingPoint
    // This method is kept for potential future enhancements
}

QJsonObject TriodeCommonCathode::exportSPICE()
{
    QJsonObject spiceNetlist;

    double vb = parameter[TRI_CC_VB]->getValue();
    double ra = parameter[TRI_CC_RA]->getValue();
    double rk = parameter[TRI_CC_RK]->getValue();
    double va = parameter[TRI_CC_VA]->getValue();
    double ia = parameter[TRI_CC_IA]->getValue();
    double vk = parameter[TRI_CC_VK]->getValue();

    spiceNetlist["title"] = "Triode Common Cathode Amplifier";
    spiceNetlist["vb"] = vb;
    spiceNetlist["ra"] = ra;
    spiceNetlist["rk"] = rk;
    spiceNetlist["va"] = va;
    spiceNetlist["ia"] = ia;
    spiceNetlist["vk"] = vk;

    return spiceNetlist;
}