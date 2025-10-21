#include "triodecommoncathode.h"

TriodeCommonCathode::TriodeCommonCathode()
{
    // Initialize circuit parameters with sensible defaults
    parameter[TRI_CC_VB] = new Parameter("Supply voltage (V)", 300.0);     // B+ supply
    parameter[TRI_CC_RK] = new Parameter("Cathode resistor (Ω)", 1000.0);   // Cathode resistor
    parameter[TRI_CC_RA] = new Parameter("Anode resistor (Ω)", 100000.0);   // Anode load resistor
    parameter[TRI_CC_RL] = new Parameter("Load impedance (Ω)", 1000000.0);  // Speaker/load impedance

    // Calculated/display parameters (read-only)
    parameter[TRI_CC_VK] = new Parameter("Bias voltage (V)", 0.0, true);   // Vk = Ia * Rk
    parameter[TRI_CC_VA] = new Parameter("Anode voltage (V)", 0.0, true);   // Va = Vb - Ia * Ra
    parameter[TRI_CC_IA] = new Parameter("Anode current (mA)", 0.0, true);  // Operating current
    parameter[TRI_CC_AR] = new Parameter("Internal resistance (Ω)", 0.0, true); // Small signal ra
    parameter[TRI_CC_GAIN] = new Parameter("Gain (unbypassed)", 0.0, true); // Voltage gain
    parameter[TRI_CC_GAIN_B] = new Parameter("Gain (bypassed)", 0.0, true); // Voltage gain with bypassed cathode
}

TriodeCommonCathode::~TriodeCommonCathode()
{
    // Parameters are managed by base class
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
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 3));
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
}

void TriodeCommonCathode::calculateCathodeLoadLine()
{
    if (device1 == nullptr) {
        return;  // No device selected
    }

    double rk = parameter[TRI_CC_RK]->getValue();  // Cathode resistor
    double vgMax = 10.0;  // Maximum grid voltage to consider
    int steps = 50;       // Number of steps for cathode load line

    cathodeLoadLineData.clear();

    for (int i = 0; i <= steps; i++) {
        double vg = (vgMax * i) / steps;  // Grid voltage from 0 to vgMax

        // Required anode current for this grid voltage: Ia = Vg / Rk
        double iaRequired = (vg / rk) * 1000.0;  // Convert to mA

        // Find anode voltage that gives this anode current for the given grid voltage
        double va = device1->anodeVoltage(iaRequired / 1000.0, -vg);  // Convert back to A for model

        if (va > 0.0) {  // Only add valid points
            cathodeLoadLineData.push_back(QPointF(va, iaRequired));
        }
    }
}

QPointF TriodeCommonCathode::findOperatingPoint()
{
    // Find intersection between anode and cathode load lines
    // Use linear interpolation to find where the two lines cross

    if (anodeLoadLineData.size() < 2 || cathodeLoadLineData.empty()) {
        // Not enough data for intersection calculation
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
                return intersection;
            }
        }
    }

    // If no intersection found, fall back to simple estimation
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

    // Calculate small-signal parameters
    // For now, use simplified approximations
    double mu = 100.0;  // Typical triode mu
    double ra_internal = 1000.0;  // Typical ra

    parameter[TRI_CC_AR]->setValue(ra_internal);

    // Calculate gains (simplified)
    double rl = parameter[TRI_CC_RL]->getValue();
    double gain_unbypassed = mu * rl / (rl + ra + (mu + 1) * rk);
    double gain_bypassed = mu * rl / (rl + ra + ra_internal);

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
    // Clear any existing circuit plots
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

    // Check if we have a device selected
    if (device1 == nullptr) {
        // No device selected - can't plot load lines
        return;
    }

    // Generate fresh load line data
    calculateAnodeLoadLine();
    calculateCathodeLoadLine();
    QPointF op = findOperatingPoint();

    // Plot anode load line (green)
    if (anodeLoadLineData.size() >= 2) {
        anodeLoadLine = new QGraphicsItemGroup();

        QPen anodePen;
        anodePen.setColor(QColor::fromRgb(0, 128, 0));  // Green for anode load line
        anodePen.setWidth(2);

        for (int i = 0; i < anodeLoadLineData.size() - 1; i++) {
            QPointF start = anodeLoadLineData[i];
            QPointF end = anodeLoadLineData[i + 1];

            QGraphicsLineItem *segment = plot->createSegment(start.x(), start.y(), end.x(), end.y(), anodePen);
            if (segment) {
                anodeLoadLine->addToGroup(segment);
            }
        }

        plot->getScene()->addItem(anodeLoadLine);
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

            QGraphicsLineItem *segment = plot->createSegment(start.x(), start.y(), end.x(), end.y(), cathodePen);
            if (segment) {
                cathodeLoadLine->addToGroup(segment);
            }
        }

        plot->getScene()->addItem(cathodeLoadLine);
    }

    // Mark operating point (red circle) - only if we have valid data
    if (op.x() >= 0 && op.y() >= 0 && cathodeLoadLineData.size() >= 2) {
        QPen opPen;
        opPen.setColor(QColor::fromRgb(255, 0, 0));  // Red for operating point
        opPen.setWidth(3);

        QGraphicsEllipseItem *opMarker = new QGraphicsEllipseItem(
            op.x() - 5, op.y() - 5, 10, 10);  // 10x10 circle centered on point

        opMarker->setPen(opPen);
        opMarker->setBrush(QBrush(QColor::fromRgb(255, 0, 0, 128)));  // Semi-transparent red fill

        plot->getScene()->addItem(opMarker);
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