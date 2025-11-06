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
}


void TriodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Input parameters (editable) - first 4
    for (int i = 0; i < 4; i++) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
        }
    }

    // Output parameters (read-only) - next 6
    for (int i = 4; i < 10; i++) {
        if (parameter[i] && labels[i] && values[i]) {
            QString labelText;
            switch (i) {
                case 4: labelText = "Bias voltage Vk (V):"; break;
                case 5: labelText = "Anode voltage Va (V):"; break;
                case 6: labelText = "Anode current Ia (mA):"; break;
                case 7: labelText = "Internal resistance ra (Ω):"; break;
                case 8: labelText = "Gain (unbypassed):"; break;
                case 9: labelText = "Gain (bypassed):"; break;
            }
            labels[i]->setText(labelText);
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(true);  // Make output parameters read-only
        }
    }

    // Hide unused parameters
    for (int i = 10; i < 16; i++) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
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
    }

    // Fallbacks if derivatives failed
    if (!std::isfinite(mu) || mu <= 0.0) mu = 100.0;
    if (!std::isfinite(ra_internal) || ra_internal <= 0.0) ra_internal = 1000.0;

    parameter[TRI_CC_AR]->setValue(ra_internal);

    // Calculate gains using device-derived mu and ra
    double rl = parameter[TRI_CC_RL]->getValue();
    double gain_unbypassed = mu * rl / (rl + ra + (mu + 1.0) * rk);
    double gain_bypassed = mu * rl / (rl + ra + ra_internal);

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

void TriodeCommonCathode::plot(Plot *plot)
{
    qInfo("Designer: plot() start. Device=%s, Vb=%.2f, Ra=%.0f, Rk=%.0f, RL=%.0f",
          device1 ? device1->getName().toStdString().c_str() : "<none>",
          parameter[TRI_CC_VB]->getValue(),
          parameter[TRI_CC_RA]->getValue(),
          parameter[TRI_CC_RK]->getValue(),
          parameter[TRI_CC_RL]->getValue());
    // Clear any existing circuit overlays (Designer-only)
    if (anodeLoadLine != nullptr) {
        plot->getScene()->removeItem(anodeLoadLine);
        delete anodeLoadLine;
        anodeLoadLine = nullptr;
    }

    if (cathodeLoadLine != nullptr) {
        plot->getScene()->removeItem(cathodeLoadLine);
        delete cathodeLoadLine;
        cathodeLoadLine = nullptr;
    }

    if (acSignalLine != nullptr) {
        plot->getScene()->removeItem(acSignalLine);
        delete acSignalLine;
        acSignalLine = nullptr;
    }

    if (opMarker != nullptr) {
        plot->getScene()->removeItem(opMarker);
        delete opMarker;
        opMarker = nullptr;
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

    qInfo("Designer: Operating point estimate Va=%.2f V, Ia=%.2f mA", op.x(), op.y());

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

    // Plot cathode load line (blue) - only if we have valid data
    if (cathodeLoadLineData.size() >= 2) {
        cathodeLoadLine = new QGraphicsItemGroup();

        QPen cathodePen;
        cathodePen.setColor(QColor::fromRgb(0, 0, 128));  // Blue for cathode load line
        cathodePen.setWidth(2);

        for (int i = 0; i < cathodeLoadLineData.size() - 1; i++) {
            QPointF start = cathodeLoadLineData[i];
            QPointF end = cathodeLoadLineData[i + 1];
            // Clamp within device limits
            start.setX(std::clamp(start.x(), 0.0, xStop));
            end.setX(std::clamp(end.x(), 0.0, xStop));
            start.setY(std::clamp(start.y(), 0.0, yStop));
            end.setY(std::clamp(end.y(), 0.0, yStop));

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

            // Span around OP: take 20% of x range but not beyond OP to 0/xStop excessively
            double dx = 0.2 * xStop;
            dx = std::min(dx, op.x());            // keep left end >= 0
            dx = std::min(dx, xStop - op.x());    // keep right end <= xStop
            if (dx > 0.0) {
                double x1 = op.x() - dx;
                double x2 = op.x() + dx;
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