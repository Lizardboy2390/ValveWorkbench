#include "pentodecommoncathode.h"

#include "valvemodel/model/device.h"

#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>

PentodeCommonCathode::PentodeCommonCathode()
{
    // Input parameters mirroring web PentodeCC defaults
    parameter[PENT_CC_VB] = new Parameter("Supply voltage (V)", 280.0);
    parameter[PENT_CC_RK] = new Parameter("Cathode resistor Rk (\u03a9)", 680.0);
    parameter[PENT_CC_RA] = new Parameter("Anode resistor Ra (\u03a9)", 82000.0);
    parameter[PENT_CC_RS] = new Parameter("Screen resistor Rs (\u03a9)", 300000.0);
    parameter[PENT_CC_RL] = new Parameter("Load impedance Rl (\u03a9)", 1000000.0);

    // Calculated values
    parameter[PENT_CC_VK]      = new Parameter("Bias point Vk (V)", 0.0);
    parameter[PENT_CC_VA]      = new Parameter("Anode voltage Va (V)", 0.0);
    parameter[PENT_CC_IA]      = new Parameter("Anode current Ia (mA)", 0.0);
    parameter[PENT_CC_VG2]     = new Parameter("Screen voltage Vg2 (V)", 0.0);
    parameter[PENT_CC_IG2]     = new Parameter("Screen current Ig2 (mA)", 0.0);
    parameter[PENT_CC_GM]      = new Parameter("gm (mA/V)", 0.0);
    parameter[PENT_CC_GAIN]    = new Parameter("Gain (unbypassed)", 0.0);
    parameter[PENT_CC_GAIN_B]  = new Parameter("Gain (bypassed)", 0.0);
}

int PentodeCommonCathode::getDeviceType(int index)
{
    Q_UNUSED(index);
    return PENTODE;
}

QTreeWidgetItem *PentodeCommonCathode::buildTree(QTreeWidgetItem *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void PentodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Inputs: first 5 fields
    for (int i = 0; i < 5; ++i) {
        if (parameter[i] && labels[i] && values[i]) {
            labels[i]->setText(parameter[i]->getName());
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', 1));
            labels[i]->setVisible(true);
            values[i]->setVisible(true);
            values[i]->setReadOnly(false);
        }
    }

    // Outputs: next 8 fields
    for (int i = 5; i <= PENT_CC_GAIN_B; ++i) {
        if (!labels[i] || !values[i]) {
            continue;
        }

        QString labelText;
        switch (i) {
        case PENT_CC_VK:      labelText = "Bias point Vk (V):"; break;
        case PENT_CC_VA:      labelText = "Anode voltage Va (V):"; break;
        case PENT_CC_IA:      labelText = "Anode current Ia (mA):"; break;
        case PENT_CC_VG2:     labelText = "Screen voltage Vg2 (V):"; break;
        case PENT_CC_IG2:     labelText = "Screen current Ig2 (mA):"; break;
        case PENT_CC_GM:      labelText = "gm (mA/V):"; break;
        case PENT_CC_GAIN:    labelText = "Gain (unbypassed):"; break;
        case PENT_CC_GAIN_B:  labelText = "Gain (bypassed):"; break;
        default: break;
        }

        labels[i]->setText(labelText);
        if (!device1) {
            values[i]->setText("N/A");
        } else if (parameter[i]) {
            int decimals = (i == PENT_CC_GAIN || i == PENT_CC_GAIN_B) ? 3 : 3;
            values[i]->setText(QString::number(parameter[i]->getValue(), 'f', decimals));
        } else {
            values[i]->setText("-");
        }

        labels[i]->setVisible(true);
        values[i]->setVisible(true);
        values[i]->setReadOnly(true);
    }

    // Hide remaining parameter slots
    for (int i = PENT_CC_GAIN_B + 1; i < 16; ++i) {
        if (labels[i] && values[i]) {
            labels[i]->setVisible(false);
            values[i]->setVisible(false);
        }
    }
}

static inline double clampDouble(double v, double lo, double hi)
{
    return std::max(lo, std::min(v, hi));
}

// Solve for screen voltage Vg2 such that cathode current (Ia+Ig2) matches
// the target ik (in mA) at given Va and Vg1, using a 1D Newton iteration
// similar to the web Model::screenVoltage implementation.
double PentodeCommonCathode::solveScreenVoltage(double ikTarget_mA,
                                                double va, double vg1) const
{
    if (!device1) {
        return 0.0;
    }

    // Initial guess for Vg2 (V)
    double vg2 = 500.0;
    const double tolerance = 1.2;

    auto cathodeCurrent_mA = [&](double vg2Guess) {
        double ia_mA = device1->anodeCurrent(va, vg1, vg2Guess);
        double ig2_mA = device1->screenCurrent(va, vg1, vg2Guess);
        if (!std::isfinite(ia_mA)) ia_mA = 0.0;
        if (!std::isfinite(ig2_mA)) ig2_mA = 0.0;
        return ia_mA + ig2_mA;
    };

    double ikTest_mA = cathodeCurrent_mA(vg2);
    double gradient = 100.0 * (cathodeCurrent_mA(vg2 + 0.01) - ikTest_mA);
    double ikErr = ikTarget_mA - ikTest_mA;

    int count = 0;
    while (std::abs(ikErr) > 0.005 && count++ < 1000) {
        if (gradient != 0.0 && std::isfinite(gradient)) {
            double vg2Next = vg2 + ikErr / gradient;
            if (vg2Next < 0.0) vg2Next = 0.0;
            // Limit step size to within [vg2/tolerance, vg2*tolerance]
            if (vg2 > 0.0) {
                double lo = vg2 / tolerance;
                double hi = vg2 * tolerance;
                vg2Next = clampDouble(vg2Next, lo, hi);
            }
            vg2 = vg2Next;
        } else {
            break;
        }

        ikTest_mA = cathodeCurrent_mA(vg2);
        gradient = 100.0 * (cathodeCurrent_mA(vg2 + 0.01) - ikTest_mA);
        ikErr = ikTarget_mA - ikTest_mA;
    }

    if (!std::isfinite(vg2)) vg2 = 0.0;
    return vg2;
}

QPointF PentodeCommonCathode::findLineIntersection(const QPointF &line1Start,
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

void PentodeCommonCathode::calculateOperatingPoint(double &va, double &ia_mA,
                                                   double &vk, double &vg2, double &ig2_mA,
                                                   double &gm_mA_V,
                                                   double &gain_unbyp, double &gain_byp) const
{
    va = ia_mA = vk = vg2 = ig2_mA = gm_mA_V = gain_unbyp = gain_byp = 0.0;

    if (!device1) {
        return;
    }

    const double vb = parameter[PENT_CC_VB]->getValue();
    const double rk = parameter[PENT_CC_RK]->getValue();
    const double ra = parameter[PENT_CC_RA]->getValue();
    const double rs = parameter[PENT_CC_RS]->getValue();
    const double rl = parameter[PENT_CC_RL]->getValue();

    if (vb <= 0.0 || rk <= 0.0 || ra <= 0.0 || rs <= 0.0 || rl <= 0.0) {
        return;
    }

    const double vgMax = device1->getVg1Max();
    const double vaMax = device1->getVaMax();

    auto solveForRatio = [&](double ratioLocal,
                             double &out_va, double &out_ia_mA,
                             double &out_vk, double &out_vg2, double &out_ig2_mA) {
        out_va = out_ia_mA = out_vk = out_vg2 = out_ig2_mA = 0.0;

        const double vaTest = vaMax * ratioLocal;

        // Screen load line in (Vg2, Ig2)
        QVector<QPointF> screenLine;
        const double ig2max_mA = vb / (rs + rk) * 1000.0;
        screenLine.push_back(QPointF(0.0, ig2max_mA));
        screenLine.push_back(QPointF(vb, 0.0));

        // Cathode line in (Vg2, Ig2) parameterized by Vg1
        QVector<QPointF> cathodeLine;
        struct CathodePoint { double vg2; double ig2; double vg1; };
        QVector<CathodePoint> cathodeMeta;

        const int steps = 100;
        for (int j = 1; j < steps; ++j) {
            const double vg1_mag = vgMax * static_cast<double>(j) / steps; // positive magnitude
            const double ik_mA = vg1_mag * 1000.0 / rk;
            if (ik_mA <= 0.0) continue;

            const double vg2_est = solveScreenVoltage(ik_mA, vaTest, -vg1_mag);
            const double ig2_here = device1->screenCurrent(vaTest, -vg1_mag, vg2_est);

            if (!std::isfinite(vg2_est) || !std::isfinite(ig2_here)) {
                continue;
            }

            cathodeLine.push_back(QPointF(vg2_est, ig2_here));
            CathodePoint meta{vg2_est, ig2_here, vg1_mag};
            cathodeMeta.push_back(meta);
        }

        if (screenLine.size() < 2 || cathodeLine.size() < 2) {
            return;
        }

        // Find intersection of screen line and cathode line
        const QPointF sStart = screenLine[0];
        const QPointF sEnd   = screenLine[1];
        int bestIndex = -1;
        QPointF bestPoint(-1.0, -1.0);
        for (int j = 0; j < cathodeLine.size() - 1; ++j) {
            const QPointF cStart = cathodeLine[j];
            const QPointF cEnd   = cathodeLine[j + 1];
            const QPointF ip     = findLineIntersection(sStart, sEnd, cStart, cEnd);
            if (ip.x() >= 0.0 && ip.y() >= 0.0) {
                bestPoint = ip;
                bestIndex = j;
                break;
            }
        }

        if (bestIndex < 0) {
            return;
        }

        out_vg2 = bestPoint.x();
        out_ig2_mA = bestPoint.y();

        const CathodePoint &cp = cathodeMeta[bestIndex];
        const double vg1Test = cp.vg1;

        // Build cathode load line in (Va, Ia) using fixed Vg1, Vg2
        QVector<QPointF> cathodeVaIa;
        cathodeVaIa.reserve(steps);
        for (int j = 1; j < steps; ++j) {
            const double va_local = vaMax * static_cast<double>(j) / steps;
            double ia_local_mA = device1->anodeCurrent(va_local, -vg1Test, out_vg2);
            if (std::isfinite(ia_local_mA) && ia_local_mA >= 0.0) {
                cathodeVaIa.push_back(QPointF(va_local, ia_local_mA));
            }
        }

        if (cathodeVaIa.size() < 2) {
            return;
        }

        // Anode load line in (Va, Ia)
        QVector<QPointF> anodeLine;
        const double iaMax_mA = vb / (ra + rk) * 1000.0;
        anodeLine.push_back(QPointF(0.0, iaMax_mA));
        anodeLine.push_back(QPointF(vb, 0.0));

        const QPointF aStart = anodeLine[0];
        const QPointF aEnd   = anodeLine[1];

        QPointF op(-1.0, -1.0);
        for (int j = 0; j < cathodeVaIa.size() - 1; ++j) {
            const QPointF cStart = cathodeVaIa[j];
            const QPointF cEnd   = cathodeVaIa[j + 1];
            const QPointF ip     = findLineIntersection(aStart, aEnd, cStart, cEnd);
            if (ip.x() >= 0.0 && ip.y() >= 0.0) {
                op = ip;
                break;
            }
        }

        if (!(op.x() >= 0.0 && op.y() >= 0.0)) {
            return;
        }

        out_va = op.x();
        out_ia_mA = op.y();

        const double ik_mA = out_ia_mA + out_ig2_mA;
        out_vk = ik_mA * rk / 1000.0; // Vk = Ik * Rk
    };

    // Two-stage solve: first approximate at ratio=0.5, then refine.
    double va1, ia1, vk1, vg21, ig21;
    solveForRatio(0.5, va1, ia1, vk1, vg21, ig21);

    const double ratio2 = (va1 > 0.0 && vb > 0.0) ? clampDouble(va1 / vb, 0.1, 0.9) : 0.5;
    solveForRatio(ratio2, va, ia_mA, vk, vg2, ig2_mA);

    if (!(va >= 0.0 && ia_mA >= 0.0)) {
        return;
    }

    // gm around OP using small ΔVg1 at fixed Va and Vg2
    double vg1_op = vk; // Vk ~ Vg1 magnitude
    const double dVg = 0.05;
    const double vgMaxLocal = device1->getVg1Max();
    double vg1_plus = clampDouble(vg1_op + dVg, 0.0, vgMaxLocal);
    double vg1_minus = clampDouble(vg1_op - dVg, 0.0, vgMaxLocal);

    double ia_plus_mA = device1->anodeCurrent(va, -vg1_minus, vg2);
    double ia_minus_mA = device1->anodeCurrent(va, -vg1_plus, vg2);
    const double dVg_span = (vg1_plus - vg1_minus);
    if (std::isfinite(ia_plus_mA) && std::isfinite(ia_minus_mA) && std::abs(dVg_span) > 1e-6) {
        gm_mA_V = (ia_minus_mA - ia_plus_mA) / dVg_span;
    }

    if (!std::isfinite(gm_mA_V) || gm_mA_V <= 0.0) {
        gm_mA_V = 0.0;
    }

    // Gains
    const double gm_A_V = gm_mA_V / 1000.0;
    const double re = (ra > 0.0 && rl > 0.0) ? (ra * rl) / (ra + rl) : (ra > 0.0 ? ra : rl);

    if (re > 0.0 && gm_A_V > 0.0) {
        const double denom_unbyp = (ra + rl) * (1.0 / gm_A_V + rk / 1000.0);
        const double denom_byp   = (ra + rl) / gm_A_V;

        if (denom_unbyp > 0.0) {
            gain_unbyp = ra * rl / denom_unbyp;
        }
        if (denom_byp > 0.0) {
            gain_byp = gm_A_V * ra * rl / (ra + rl);
        }
    }

    if (!std::isfinite(gain_unbyp)) gain_unbyp = 0.0;
    if (!std::isfinite(gain_byp)) gain_byp = 0.0;
}

void PentodeCommonCathode::update(int index)
{
    Q_UNUSED(index);

    double va = 0.0;
    double ia_mA = 0.0;
    double vk = 0.0;
    double vg2 = 0.0;
    double ig2_mA = 0.0;
    double gm_mA_V = 0.0;
    double gain_unbyp = 0.0;
    double gain_byp = 0.0;

    calculateOperatingPoint(va, ia_mA, vk, vg2, ig2_mA, gm_mA_V, gain_unbyp, gain_byp);

    parameter[PENT_CC_VK]->setValue(vk);
    parameter[PENT_CC_VA]->setValue(va);
    parameter[PENT_CC_IA]->setValue(ia_mA);
    parameter[PENT_CC_VG2]->setValue(vg2);
    parameter[PENT_CC_IG2]->setValue(ig2_mA);
    parameter[PENT_CC_GM]->setValue(gm_mA_V);
    parameter[PENT_CC_GAIN]->setValue(gain_unbyp);
    parameter[PENT_CC_GAIN_B]->setValue(gain_byp);
}

void PentodeCommonCathode::plot(Plot *plot)
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

    double va = 0.0;
    double ia_mA = 0.0;
    double vk = 0.0;
    double vg2 = 0.0;
    double ig2_mA = 0.0;
    double gm_mA_V = 0.0;
    double gain_unbyp = 0.0;
    double gain_byp = 0.0;

    calculateOperatingPoint(va, ia_mA, vk, vg2, ig2_mA, gm_mA_V, gain_unbyp, gain_byp);
    if (!(va >= 0.0 && ia_mA >= 0.0)) {
        return;
    }

    const double vb = parameter[PENT_CC_VB]->getValue();
    const double rk = parameter[PENT_CC_RK]->getValue();
    const double ra = parameter[PENT_CC_RA]->getValue();

    // Anode load line (green)
    QVector<QPointF> anodeData;
    const double iaMax_mA = vb / (ra + rk) * 1000.0;
    anodeData.push_back(QPointF(0.0, iaMax_mA));
    anodeData.push_back(QPointF(vb, 0.0));

    anodeLoadLine = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 128, 0));
        pen.setWidth(2);
        for (int i = 0; i < anodeData.size() - 1; ++i) {
            const QPointF s = anodeData[i];
            const QPointF e = anodeData[i + 1];
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

    // Cathode load line (blue) in Va-Ia at found Vg1/Vg2
    const double vaMaxLocal = device1->getVaMax();
    const double vg1_mag = vk; // Vk ~ Vg1 magnitude
    QVector<QPointF> cathodeData;
    cathodeData.reserve(100);
    for (int j = 1; j < 100; ++j) {
        const double va_local = vaMaxLocal * static_cast<double>(j) / 100.0;
        double ia_local_mA = device1->anodeCurrent(va_local, -vg1_mag, vg2);
        if (std::isfinite(ia_local_mA) && ia_local_mA >= 0.0) {
            cathodeData.push_back(QPointF(va_local, ia_local_mA));
        }
    }

    if (cathodeData.size() >= 2) {
        cathodeLoadLine = new QGraphicsItemGroup();
        QPen pen;
        pen.setColor(QColor::fromRgb(0, 0, 128));
        pen.setWidth(2);

        for (int i = 0; i < cathodeData.size() - 1; ++i) {
            const QPointF s = cathodeData[i];
            const QPointF e = cathodeData[i + 1];
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

    const double paMaxW = device1->getPaMax();
    if (paMaxW > 0.0) {
        acSignalLine = new QGraphicsItemGroup();
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

        if (!acSignalLine->childItems().isEmpty()) {
            plot->getScene()->addItem(acSignalLine);
        } else {
            delete acSignalLine;
            acSignalLine = nullptr;
        }
    }

    // Operating point marker (red cross)
    opMarker = new QGraphicsItemGroup();
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(255, 0, 0));
        pen.setWidth(2);
        const double d = 5.0;
        if (auto *h = plot->createSegment(va - d, ia_mA, va + d, ia_mA, pen)) opMarker->addToGroup(h);
        if (auto *v = plot->createSegment(va, ia_mA - d, va, ia_mA + d, pen)) opMarker->addToGroup(v);

        if (!opMarker->childItems().isEmpty()) {
            plot->getScene()->addItem(opMarker);
        } else {
            delete opMarker;
            opMarker = nullptr;
        }
    }
}
