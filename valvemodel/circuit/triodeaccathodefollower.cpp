#include "triodeaccathodefollower.h"

#include "valvemodel/model/device.h"

TriodeACCathodeFollower::TriodeACCathodeFollower()
{
    // Input parameters
    parameter[TRI_ACCF_VB] = new Parameter("Supply voltage (V)", 300.0);
    parameter[TRI_ACCF_RB] = new Parameter("Anode resistor Rb (\u03a9)", 620.0);
    parameter[TRI_ACCF_RK] = new Parameter("Cathode resistor Rk (\u03a9)", 47000.0);
    parameter[TRI_ACCF_RL] = new Parameter("Load impedance Rl (\u03a9)", 100000.0);

    // Calculated values
    parameter[TRI_ACCF_VG] = new Parameter("Bias point Vg (V)", 0.0);
    parameter[TRI_ACCF_VK] = new Parameter("Cathode voltage (V)", 0.0);
    parameter[TRI_ACCF_IK] = new Parameter("Cathode current (mA)", 0.0);
    parameter[TRI_ACCF_RO] = new Parameter("Output impedance (\u03a9)", 0.0);
}

int TriodeACCathodeFollower::getDeviceType(int index)
{
    // Single triode device for this circuit
    if (index == 1) {
        return TRIODE;
    }
    return -1;
}

void TriodeACCathodeFollower::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first 4 fields
    for (int i = 0; i < 4; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            values[i]->setReadOnly(false);
        }
    }

    // Outputs: next 4 fields
    for (int i = 4; i < 8; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            QString labelText;
            switch (i) {
            case TRI_ACCF_VG: labelText = "Bias point Vg (V):"; break;
            case TRI_ACCF_VK: labelText = "Cathode voltage Vk (V):"; break;
            case TRI_ACCF_IK: labelText = "Cathode current Ik (mA):"; break;
            case TRI_ACCF_RO: labelText = "Output impedance (\u03a9):"; break;
            default: break;
            }

            labels[i]->setText(labelText);
            if (device1 == nullptr) {
                values[i]->setText("N/A");
            } else {
                if (i == TRI_ACCF_RO) {
                    values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 0));
                } else {
                    values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 2));
                }
            }

            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(true);
        }
    }

    // Hide any remaining parameter slots
    for (int i = 8; i < 16; ++i) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
        }
    }
}

void TriodeACCathodeFollower::calculateLoadLines()
{
    biasLoadLineData.clear();
    cathodeLoadLineData.clear();

    if (!device1) {
        return;
    }

    const double vb = parameter[TRI_ACCF_VB]->getValue();
    const double rb = parameter[TRI_ACCF_RB]->getValue();
    const double rk = parameter[TRI_ACCF_RK]->getValue();

    if (vb <= 0.0 || rb <= 0.0 || rk <= 0.0) {
        return;
    }

    // Anode/bias load line: from (0, Ia_max) to (Vb, 0)
    const double iaMax_mA = vb / (rb + rk) * 1000.0; // mA
    biasLoadLineData.push_back(QPointF(0.0, iaMax_mA));
    biasLoadLineData.push_back(QPointF(vb, 0.0));

    // Cathode load line in Va-Ik space, using model anodeVoltage at fixed grid bias
    const double vgMax = device1->getVg1Max(); // positive grid span used as magnitude
    const int steps = 100;

    for (int j = 1; j <= steps; ++j) {
        const double vg_mag = vgMax * static_cast<double>(j) / steps; // V
        const double ia_mA = vg_mag * 1000.0 / rb;                      // from Ik = Vg/Rb
        if (ia_mA <= 0.0) {
            continue;
        }

        const double va = device1->anodeVoltage(ia_mA, -vg_mag);
        if (std::isfinite(va) && va > 0.0001) {
            cathodeLoadLineData.push_back(QPointF(va, ia_mA));
        }
    }
}

QPointF TriodeACCathodeFollower::findLineIntersection(QPointF line1Start, QPointF line1End,
                                                      QPointF line2Start, QPointF line2End) const
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

QPointF TriodeACCathodeFollower::findOperatingPoint()
{
    calculateLoadLines();

    if (biasLoadLineData.size() < 2 || cathodeLoadLineData.size() < 2) {
        return QPointF(-1.0, -1.0);
    }

    // Single segment for bias line, scan over cathode line segments
    const QPointF aStart = biasLoadLineData[0];
    const QPointF aEnd   = biasLoadLineData[1];

    for (int j = 0; j < cathodeLoadLineData.size() - 1; ++j) {
        const QPointF cStart = cathodeLoadLineData[j];
        const QPointF cEnd   = cathodeLoadLineData[j + 1];
        const QPointF ip     = findLineIntersection(aStart, aEnd, cStart, cEnd);
        if (ip.x() >= 0.0 && ip.y() >= 0.0) {
            return ip;
        }
    }

    return QPointF(-1.0, -1.0);
}

void TriodeACCathodeFollower::computeSmallSignal(double va_k, double ia_mA,
                                                 double &vg, double &vk,
                                                 double &gm_mA_V, double &ro_ohm) const
{
    vg = 0.0;
    vk = 0.0;
    gm_mA_V = 0.0;
    ro_ohm = 0.0;

    if (!device1) {
        return;
    }

    const double rb = parameter[TRI_ACCF_RB]->getValue();
    const double rk = parameter[TRI_ACCF_RK]->getValue();

    if (rb <= 0.0 || rk <= 0.0 || ia_mA <= 0.0) {
        return;
    }

    // DC voltages at operating point (following the web implementation)
    vk = ia_mA * (rk + rb) / 1000.0; // V
    vg = ia_mA * rb / 1000.0;        // V

    // Small-signal gm around this operating point using finite differences on grid voltage
    const double vgMax = device1->getVg1Max();
    const double step  = 0.05; // 50 mV step

    const double vg_plus_mag  = std::clamp(vg + step, 0.0, vgMax);
    const double vg_minus_mag = std::clamp(vg - step, 0.0, vgMax);

    if (vg_plus_mag <= vg_minus_mag) {
        return;
    }

    const double ia_plus_mA  = device1->anodeCurrent(va_k, -vg_plus_mag);
    const double ia_minus_mA = device1->anodeCurrent(va_k, -vg_minus_mag);

    const double dVg = vg_plus_mag - vg_minus_mag;
    if (std::abs(dVg) > 1e-6) {
        gm_mA_V = (ia_minus_mA - ia_plus_mA) / dVg; // mA/V, positive slope
    }

    if (gm_mA_V > 0.0) {
        // ro in ohms, using gm in mA/V -> gm[A/V] = gm_mA_V / 1000
        ro_ohm = 1000.0 / gm_mA_V;
    }
}

void TriodeACCathodeFollower::update(int index)
{
    Q_UNUSED(index);

    if (!device1) {
        return;
    }

    const QPointF op = findOperatingPoint();
    if (!(op.x() >= 0.0 && op.y() >= 0.0)) {
        // Reset outputs if OP cannot be found
        parameter[TRI_ACCF_VG]->setValue(0.0);
        parameter[TRI_ACCF_VK]->setValue(0.0);
        parameter[TRI_ACCF_IK]->setValue(0.0);
        parameter[TRI_ACCF_RO]->setValue(0.0);
        return;
    }

    const double va_k = op.x();       // Va at OP
    const double ik_mA = op.y();      // cathode current in mA

    double vg = 0.0;
    double vk = 0.0;
    double gm_mA_V = 0.0;
    double ro_ohm = 0.0;

    computeSmallSignal(va_k, ik_mA, vg, vk, gm_mA_V, ro_ohm);

    parameter[TRI_ACCF_VG]->setValue(vg);
    parameter[TRI_ACCF_VK]->setValue(vk);
    parameter[TRI_ACCF_IK]->setValue(ik_mA);
    parameter[TRI_ACCF_RO]->setValue(ro_ohm);
}

void TriodeACCathodeFollower::plot(Plot *plot)
{
    if (!device1) {
        return;
    }

    // Clear existing overlays specific to circuits
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

    if (opMarker != nullptr) {
        plot->getScene()->removeItem(opMarker);
        delete opMarker;
        opMarker = nullptr;
    }

    // Set axes if scene is empty so Designer shares the same domain as the model plot
    const double vaMax = device1->getVaMax();
    const double iaMax = device1->getIaMax();
    const double xMajor = std::max(5.0, vaMax / 10.0);
    const double yMajor = std::max(0.5, iaMax / 10.0);

    if (plot->getScene()->items().isEmpty()) {
        plot->setAxes(0.0, vaMax, xMajor, 0.0, iaMax, yMajor);
    }

    // Rebuild load lines and operating point
    calculateLoadLines();
    const QPointF op = findOperatingPoint();

    // Bias/anode load line (green)
    if (biasLoadLineData.size() >= 2) {
        anodeLoadLine = new QGraphicsItemGroup();
        QPen pen; pen.setColor(QColor::fromRgb(0, 128, 0)); pen.setWidth(2);

        for (int i = 0; i < biasLoadLineData.size() - 1; ++i) {
            const QPointF s = biasLoadLineData[i];
            const QPointF e = biasLoadLineData[i + 1];
            QGraphicsLineItem *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen);
            if (seg) {
                anodeLoadLine->addToGroup(seg);
            }
        }
        if (!anodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(anodeLoadLine);
        } else {
            delete anodeLoadLine; anodeLoadLine = nullptr;
        }
    }

    // Cathode/bias line (blue)
    if (cathodeLoadLineData.size() >= 2) {
        cathodeLoadLine = new QGraphicsItemGroup();
        QPen pen; pen.setColor(QColor::fromRgb(0, 0, 128)); pen.setWidth(2);

        for (int i = 0; i < cathodeLoadLineData.size() - 1; ++i) {
            const QPointF s = cathodeLoadLineData[i];
            const QPointF e = cathodeLoadLineData[i + 1];
            QGraphicsLineItem *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen);
            if (seg) {
                cathodeLoadLine->addToGroup(seg);
            }
        }
        if (!cathodeLoadLine->childItems().isEmpty()) {
            plot->getScene()->addItem(cathodeLoadLine);
        } else {
            delete cathodeLoadLine; cathodeLoadLine = nullptr;
        }
    }

    // Operating point marker (red cross)
    if (op.x() >= 0.0 && op.y() >= 0.0) {
        opMarker = new QGraphicsItemGroup();
        QPen pen; pen.setColor(QColor::fromRgb(255, 0, 0)); pen.setWidth(2);

        const double d = 5.0;
        QGraphicsLineItem *h = plot->createSegment(op.x() - d, op.y(), op.x() + d, op.y(), pen);
        QGraphicsLineItem *v = plot->createSegment(op.x(), op.y() - d, op.x(), op.y() + d, pen);
        if (h) opMarker->addToGroup(h);
        if (v) opMarker->addToGroup(v);

        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker; opMarker = nullptr;
        }
    }
}

QTreeWidgetItem *TriodeACCathodeFollower::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    // Designer-only circuit; it does not appear in the project tree.
    return nullptr;
}
