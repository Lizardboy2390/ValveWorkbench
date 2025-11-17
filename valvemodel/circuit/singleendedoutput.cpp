#include "singleendedoutput.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <QGraphicsPolygonItem>

#include <algorithm>
#include <cmath>

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
        }
    }

    // Outputs: remaining fields after SE_HEADROOM
    for (int i = SE_VK; i <= SE_PHEAD; ++i) {
        if (!labels[i] || !values[i]) continue;

        QString labelText;
        switch (i) {
        case SE_VK:   labelText = "Bias point Vk (V):"; break;
        case SE_IK:   labelText = "Cathode current (mA):"; break;
        case SE_RK:   labelText = "Cathode resistor (\u03a9):"; break;
        case SE_POUT: labelText = "Max output power (W):"; break;
        case SE_PHEAD:labelText = "Power at headroom (W):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = (i == SE_POUT || i == SE_PHEAD) ? 3 : 3;
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);
    }

    // Hide remaining parameter slots
    for (int i = SE_PHEAD + 1; i < 16; ++i) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
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
        return;
    }

    const double vb  = parameter[SE_VB]->getValue();
    const double vs  = parameter[SE_VS]->getValue();
    const double ia        = parameter[SE_IA]->getValue(); // mA
    const double raa       = parameter[SE_RA]->getValue();
    const double headroom  = parameter[SE_HEADROOM]->getValue(); // Vpk at anode

    if (vb <= 0.0 || raa <= 0.0 || ia <= 0.0) {
        parameter[SE_VK]->setValue(0.0);
        parameter[SE_IK]->setValue(0.0);
        parameter[SE_RK]->setValue(0.0);
        parameter[SE_POUT]->setValue(0.0);
        parameter[SE_PHEAD]->setValue(0.0);
        return;
    }

    const double vg1Max = device1->getVg1Max();
    const double vaMax = device1->getVaMax();

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

    // Find Vk (grid bias) such that Ia(Vb, -Vk, Vs) ~= ia
    double bestVg1 = 0.0;
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

    double ik_mA = ia;
    if (device1->getDeviceType() == PENTODE) {
        ik_mA += device1->screenCurrent(vb, -bestVg1, vs) * 1000.0;
    }

    double rk_ohms = 0.0;
    if (ik_mA > 0.0) {
        rk_ohms = 1000.0 * bestVg1 / ik_mA;
    }

    parameter[SE_VK]->setValue(bestVg1);
    parameter[SE_IK]->setValue(ik_mA);
    parameter[SE_RK]->setValue(rk_ohms);
    parameter[SE_POUT]->setValue(pout_W);

    // Simple power-at-headroom helper using linear load assumption around OP.
    double phead_W = 0.0;
    if (headroom > 0.0 && raa > 0.0) {
        // Treat SE_HEADROOM as anode peak swing; output power is Vpk^2 / (2 * Ra).
        phead_W = (headroom * headroom) / (2.0 * raa);
    }
    parameter[SE_PHEAD]->setValue(phead_W);
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

    // Draw a simple headroom segment around the operating point along the AC load line
    if (headroom > 0.0) {
        double vaMin = vb - headroom;
        double vaMax2 = vb + headroom;

        // Clamp to the plotted Va range instead of skipping when out of bounds
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
            if (auto *seg = plot->createSegment(vaMin, iaHigh, vaMax2, iaLow, pen)) {
                anodeLoadLine->addToGroup(seg);
            }

            // Build a filled polygon in scene coordinates under the headroom line,
            // using the same mapping as Plot::createSegment.
            const double xScale = PLOT_WIDTH  / (vaMax - 0.0);
            const double yScale = PLOT_HEIGHT / (iaMax - 0.0);

            const double sx1 = (vaMin  - 0.0) * xScale;
            const double sy1 = PLOT_HEIGHT - (iaHigh - 0.0) * yScale;
            const double sx2 = (vaMax2 - 0.0) * xScale;
            const double sy2 = PLOT_HEIGHT - (iaLow  - 0.0) * yScale;
            const double sx3 = sx2;
            const double sy3 = PLOT_HEIGHT - (0.0    - 0.0) * yScale;
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
}
