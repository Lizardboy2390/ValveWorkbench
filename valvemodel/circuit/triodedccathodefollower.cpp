#include "triodedccathodefollower.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>

TriodeDCCathodeFollower::TriodeDCCathodeFollower()
{
    // Input parameters matching the web DCCathodeFollower
    parameter[TRI_DCCF_VB] = new Parameter("Supply voltage (V)", 300.0);
    parameter[TRI_DCCF_RK] = new Parameter("Cathode resistor Rk (\u03a9)", 100000.0);
    parameter[TRI_DCCF_RA] = new Parameter("Anode resistor Ra (\u03a9)", 0.0);
    parameter[TRI_DCCF_RL] = new Parameter("Load impedance Rl (\u03a9)", 82000.0);

    // Calculated values (stage 1 bias, gain, and follower DC state)
    parameter[TRI_DCCF_VK]   = new Parameter("Bias point Vk (V)", 0.0);
    parameter[TRI_DCCF_VA]   = new Parameter("Anode voltage (V)", 0.0);
    parameter[TRI_DCCF_IA]   = new Parameter("Anode current (mA)", 0.0);
    parameter[TRI_DCCF_GAIN] = new Parameter("Gain", 0.0);
    parameter[TRI_DCCF_VK2]  = new Parameter("Follower voltage (V)", 0.0);
    parameter[TRI_DCCF_IK2]  = new Parameter("Follower current (mA)", 0.0);
}

int TriodeDCCathodeFollower::getDeviceType(int index)
{
    Q_UNUSED(index);
    // Single triode device
    return TRIODE;
}

QTreeWidgetItem *TriodeDCCathodeFollower::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    // Designer-only circuit; it does not appear in the project tree.
    return nullptr;
}

void TriodeDCCathodeFollower::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first 4 fields, editable
    for (int i = 0; i < 4; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);
        }
    }

    // Outputs: next 6 fields, read-only
    for (int i = 4; i < 10; ++i) {
        if (!labels[i] || !values[i]) {
            continue;
        }

        QString labelText;
        switch (i) {
        case TRI_DCCF_VK:   labelText = "Bias point Vk (V):"; break;
        case TRI_DCCF_VA:   labelText = "Anode voltage Va (V):"; break;
        case TRI_DCCF_IA:   labelText = "Anode current Ia (mA):"; break;
        case TRI_DCCF_GAIN: labelText = "Gain:"; break;
        case TRI_DCCF_VK2:  labelText = "Follower voltage Vk2 (V):"; break;
        case TRI_DCCF_IK2:  labelText = "Follower current Ik2 (mA):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = (i == TRI_DCCF_GAIN) ? 3 : 3;
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);
    }

    // Hide remaining parameter slots
    for (int i = 10; i < 16; ++i) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
        }
    }
}

void TriodeDCCathodeFollower::calculateLoadLines(QVector<QPointF> &anodeData,
                                                 QVector<QPointF> &cathodeData)
{
    anodeData.clear();
    cathodeData.clear();

    if (!device1) {
        return;
    }

    const double vb = parameter[TRI_DCCF_VB]->getValue();
    const double rk = parameter[TRI_DCCF_RK]->getValue();
    const double ra = parameter[TRI_DCCF_RA]->getValue();

    if (vb <= 0.0 || rk <= 0.0 || ra <= 0.0) {
        return;
    }

    // Anode DC load line: from (0, Ia_max) to (Vb, 0)
    const double iaMax_mA = vb / (ra + rk) * 1000.0; // mA
    anodeData.push_back(QPointF(0.0, iaMax_mA));
    anodeData.push_back(QPointF(vb, 0.0));

    // Cathode load line in Va-Ia space, using model anodeVoltage at fixed grid bias
    const double vgMax = device1->getVg1Max();
    const int steps = 100;

    for (int j = 1; j <= steps; ++j) {
        const double vg = vgMax * static_cast<double>(j) / steps; // V (positive magnitude)
        const double ia_mA = vg * 1000.0 / rk;                    // Ia from Vg/Rk (mA)
        if (ia_mA <= 0.0) {
            continue;
        }

        const double va = device1->anodeVoltage(ia_mA, -vg);
        if (std::isfinite(va) && va > 0.0001) {
            cathodeData.push_back(QPointF(va, ia_mA));
        }
    }
}

QPointF TriodeDCCathodeFollower::findLineIntersection(const QPointF &line1Start,
                                                      const QPointF &line1End,
                                                      const QPointF &line2Start,
                                                      const QPointF &line2End) const
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

QPointF TriodeDCCathodeFollower::findOperatingPoint(const QVector<QPointF> &anodeData,
                                                    const QVector<QPointF> &cathodeData) const
{
    if (anodeData.size() < 2 || cathodeData.size() < 2) {
        return QPointF(-1.0, -1.0);
    }

    const QPointF aStart = anodeData[0];
    const QPointF aEnd   = anodeData[1];

    for (int j = 0; j < cathodeData.size() - 1; ++j) {
        const QPointF cStart = cathodeData[j];
        const QPointF cEnd   = cathodeData[j + 1];
        const QPointF ip     = findLineIntersection(aStart, aEnd, cStart, cEnd);
        if (ip.x() >= 0.0 && ip.y() >= 0.0) {
            return ip;
        }
    }

    return QPointF(-1.0, -1.0);
}

void TriodeDCCathodeFollower::update(int index)
{
    Q_UNUSED(index);

    if (!device1) {
        parameter[TRI_DCCF_VK]->setValue(0.0);
        parameter[TRI_DCCF_VA]->setValue(0.0);
        parameter[TRI_DCCF_IA]->setValue(0.0);
        parameter[TRI_DCCF_GAIN]->setValue(0.0);
        parameter[TRI_DCCF_VK2]->setValue(0.0);
        parameter[TRI_DCCF_IK2]->setValue(0.0);
        return;
    }

    const double vb = parameter[TRI_DCCF_VB]->getValue();
    const double rk = parameter[TRI_DCCF_RK]->getValue();
    const double ra = parameter[TRI_DCCF_RA]->getValue();
    const double rl = parameter[TRI_DCCF_RL]->getValue();

    if (vb <= 0.0 || rk <= 0.0 || ra <= 0.0 || rl <= 0.0) {
        parameter[TRI_DCCF_VK]->setValue(0.0);
        parameter[TRI_DCCF_VA]->setValue(0.0);
        parameter[TRI_DCCF_IA]->setValue(0.0);
        parameter[TRI_DCCF_GAIN]->setValue(0.0);
        parameter[TRI_DCCF_VK2]->setValue(0.0);
        parameter[TRI_DCCF_IK2]->setValue(0.0);
        return;
    }

    QVector<QPointF> anodeData;
    QVector<QPointF> cathodeData;
    calculateLoadLines(anodeData, cathodeData);
    const QPointF op = findOperatingPoint(anodeData, cathodeData);
    if (!(op.x() >= 0.0 && op.y() >= 0.0)) {
        parameter[TRI_DCCF_VK]->setValue(0.0);
        parameter[TRI_DCCF_VA]->setValue(0.0);
        parameter[TRI_DCCF_IA]->setValue(0.0);
        parameter[TRI_DCCF_GAIN]->setValue(0.0);
        parameter[TRI_DCCF_VK2]->setValue(0.0);
        parameter[TRI_DCCF_IK2]->setValue(0.0);
        return;
    }

    const double vaStage    = op.x();          // Va (V)
    const double iaStage_mA = op.y();          // Ia (mA)
    const double iaStage_A  = iaStage_mA / 1000.0;
    const double vkStage    = iaStage_A * rk;  // Vk (V)

    // Follower DC operating point (stage 2), per web reference
    const double vkFollower    = vb - vaStage;             // Vk2 (V)
    const double ikFollower_mA = (vaStage * 1000.0) / rl;  // Ik2 (mA)

    parameter[TRI_DCCF_VK]->setValue(vkStage);
    parameter[TRI_DCCF_VA]->setValue(vaStage);
    parameter[TRI_DCCF_IA]->setValue(iaStage_mA);
    parameter[TRI_DCCF_VK2]->setValue(vkFollower);
    parameter[TRI_DCCF_IK2]->setValue(ikFollower_mA);

    // Analytic gain (loadLines-style): mu, ar, re, ark, gainBP
    double mu = 0.0;
    double ar = 0.0;          // Effective anode resistance (Ohms)

    if (std::isfinite(vaStage) && std::isfinite(iaStage_A)) {
        const double ia_mA = iaStage_mA;
        const double vg0   = -iaStage_A * rk;  // self-bias grid voltage

        // Steps for numerical derivatives
        const double dVg    = std::max(0.01, std::abs(vg0) * 0.02);

        const double vgMin  = -device1->getVg1Max();
        const double vgPlus = std::clamp(vg0 + dVg, vgMin, 0.0);
        const double vgMinus= std::clamp(vg0 - dVg, vgMin, 0.0);

        // mu = -dVa/dVg at constant Ia (unitless)
        const double va_plusVg  = device1->anodeVoltage(ia_mA, vgPlus);
        const double va_minusVg = device1->anodeVoltage(ia_mA, vgMinus);
        if (std::isfinite(va_plusVg) && std::isfinite(va_minusVg) && (vgPlus - vgMinus) != 0.0) {
            const double dVa_dVg = (va_plusVg - va_minusVg) / (vgPlus - vgMinus);
            mu = -dVa_dVg;
        }

        // ar from finite difference around supply point (web-style)
        const double ia1_mA = device1->anodeCurrent(vb, -vkStage);
        const double ia2_mA = device1->anodeCurrent(vb - 10.0, -vkStage);
        const double dIa_mA = ia1_mA - ia2_mA;
        if (std::isfinite(ia1_mA) && std::isfinite(ia2_mA) && std::abs(dIa_mA) > 1e-9) {
            // JS uses ar = 10 / (ia1 - ia2) assuming Ia in amps; here Ia is mA.
            // Convert to Ohms: 10 V / ((dIa_mA / 1000) A) = 10000 / dIa_mA.
            ar = 10000.0 / dIa_mA;
        }
    }

    if (!std::isfinite(mu) || mu <= 0.0) {
        mu = 100.0;
    }
    if (!std::isfinite(ar) || ar <= 0.0) {
        ar = 1000.0;
    }

    // Effective load combination and analytic gains (same formulas as web DCCathodeFollower)
    const double re  = (ra > 0.0 && rl > 0.0) ? (ra * rl) / (ra + rl) : (ra > 0.0 ? ra : rl);
    const double ark = ar + rk * (mu + 1.0);
    double gain_bypassed = 0.0;

    if (re > 0.0 && std::isfinite(re)) {
        gain_bypassed = mu * re / (re + ar);
    }
    if (!std::isfinite(gain_bypassed)) {
        gain_bypassed   = 0.0;
    }

    parameter[TRI_DCCF_GAIN]->setValue(gain_bypassed);
}

void TriodeDCCathodeFollower::plot(Plot *plot)
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

    QVector<QPointF> anodeData;
    QVector<QPointF> cathodeData;
    calculateLoadLines(anodeData, cathodeData);
    const QPointF op = findOperatingPoint(anodeData, cathodeData);

    // Anode load line (green)
    if (anodeData.size() >= 2) {
        anodeLoadLine = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 128, 0));
        pen.setWidth(2);

        for (int i = 0; i < anodeData.size() - 1; ++i) {
            const QPointF s = anodeData[i];
            const QPointF e = anodeData[i + 1];
            QGraphicsLineItem *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen);
            if (seg) {
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

    // Cathode load line (blue)
    if (cathodeData.size() >= 2) {
        cathodeLoadLine = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 0, 128));
        pen.setWidth(2);

        for (int i = 0; i < cathodeData.size() - 1; ++i) {
            const QPointF s = cathodeData[i];
            const QPointF e = cathodeData[i + 1];
            QGraphicsLineItem *seg = plot->createSegment(s.x(), s.y(), e.x(), e.y(), pen);
            if (seg) {
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

    // Follower load line (cyan), using vb and rl
    const double vb = parameter[TRI_DCCF_VB]->getValue();
    const double rl = parameter[TRI_DCCF_RL]->getValue();
    if (vb > 0.0 && rl > 0.0) {
        acSignalLine = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 191, 191));
        pen.setWidth(2);

        const double ik2_mA = vb * 1000.0 / rl;
        // Simple line from (0, Ik2) to (Vb, 0)
        QGraphicsLineItem *seg = plot->createSegment(0.0, ik2_mA, vb, 0.0, pen);
        if (seg) {
            acSignalLine->addToGroup(seg);
        }
        if (!acSignalLine->childItems().isEmpty()) {
            plot->getScene()->addItem(acSignalLine);
        } else {
            delete acSignalLine;
            acSignalLine = nullptr;
        }
    }

    // Operating point marker (red cross) for stage 1
    if (op.x() >= 0.0 && op.y() >= 0.0) {
        opMarker = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(255, 0, 0));
        pen.setWidth(2);

        const double d = 5.0;
        QGraphicsLineItem *h = plot->createSegment(op.x() - d, op.y(), op.x() + d, op.y(), pen);
        QGraphicsLineItem *v = plot->createSegment(op.x(), op.y() - d, op.x(), op.y() + d, pen);
        if (h) opMarker->addToGroup(h);
        if (v) opMarker->addToGroup(v);

        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker;
            opMarker = nullptr;
        }
    }
}

