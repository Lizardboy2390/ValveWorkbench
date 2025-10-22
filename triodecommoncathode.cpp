#include "triodecommoncathode.h"
#include "valvemodel/ui/parameter.h"
#include "valvemodel/model/device.h"
#include <QPointF>
#include <QVector>
#include <QtMath>

TriodeCommonCathode::TriodeCommonCathode()
{
    // Initialize circuit parameters
    parameter[0] = new Parameter("Supply Voltage (V)", 300.0);  // VB
    parameter[1] = new Parameter("Anode Resistor (kΩ)", 100.0);   // RA
    parameter[2] = new Parameter("Cathode Resistor (kΩ)", 1.0);    // RK
    parameter[3] = new Parameter("Load Resistor (kΩ)", 100.0);     // RL

    // Initialize remaining parameters as null or defaults
    for (int i = 4; i < 16; i++) {
        parameter[i] = nullptr;
    }
}

TriodeCommonCathode::~TriodeCommonCathode()
{
    // Clean up parameters
    for (int i = 0; i < 16; i++) {
        if (parameter[i]) {
            delete parameter[i];
        }
    }
}

int TriodeCommonCathode::getDeviceType(int index)
{
    if (index == 0) {
        return MODEL_TRIODE;  // Triode for common cathode
    }
    return -1;  // No device for other indices
}

void TriodeCommonCathode::plot(Plot *plot)
{
    // Clear existing plot items
    if (Circuit::anodeLoadLine) {
        plot->remove(Circuit::anodeLoadLine);
        delete Circuit::anodeLoadLine;
        Circuit::anodeLoadLine = nullptr;
    }

    if (!device1) return;

    // Plot anode load line (green)
    QPen anodePen;
    anodePen.setColor(QColor::fromRgb(0, 128, 0));  // Green

    Circuit::anodeLoadLine = new QGraphicsItemGroup();
    for (int i = 0; i < anodeLoadLine.size() - 1; i++) {
        QPointF p1 = anodeLoadLine[i];
        QPointF p2 = anodeLoadLine[i + 1];
        QGraphicsLineItem *segment = plot->createSegment(p1.x(), p1.y(), p2.x(), p2.y(), anodePen);
        if (segment) {
            Circuit::anodeLoadLine->addToGroup(segment);
        }
    }
    plot->add(Circuit::anodeLoadLine);
}

void TriodeCommonCathode::update(int index)
{
    calculateOperatingPoint();
}

void TriodeCommonCathode::calculateOperatingPoint()
{
    if (!device1) return;

    double vb = getParameter(0);  // Supply voltage
    double ra = getParameter(1) * 1000;  // Anode resistor in ohms
    double rk = getParameter(2) * 1000;  // Cathode resistor in ohms
    double rl = getParameter(3) * 1000;  // Load resistor in ohms

    // Calculate anode load line
    calculateAnodeLoadLine(vb, ra, rl);

    // Calculate cathode load line
    calculateCathodeLoadLine(rk);

    // Find intersection (operating point)
    QPointF op = findOperatingPoint();
    operatingPoint = op;
}

void TriodeCommonCathode::calculateAnodeLoadLine(double vb, double ra, double rl)
{
    anodeLoadLine.clear();

    double totalR = ra + rl;
    double iaMax = vb / totalR * 1000;  // Convert to mA

    // Linear load line from (0, iaMax) to (vb, 0)
    for (int i = 0; i <= 100; i++) {
        double va = vb * i / 100.0;
        double ia = iaMax * (1.0 - i / 100.0);
        anodeLoadLine.append(QPointF(va, ia));
    }
}

void TriodeCommonCathode::calculateCathodeLoadLine(double rk)
{
    cathodeLoadLine.clear();

    if (!device1) return;

    double vgMax = device1->getVg1Max();
    int steps = 100;

    for (int j = 1; j <= steps; j++) {
        double vg = vgMax * j / 100.0;
        double ia = vg * 1000.0 / rk;  // Ia = Vg / Rk in mA

        // Find Va such that anodeCurrent(Va, -vg) = ia
        double va = device1->anodeVoltage(ia / 1000.0, -vg);  // Convert ia to A

        if (va > 0.001) {
            cathodeLoadLine.append(QPointF(va, ia));
        }
    }
}

QPointF TriodeCommonCathode::findOperatingPoint()
{
    QPointF intersection(0, 0);

    // Find intersection between anode and cathode load lines
    for (int i = 0; i < anodeLoadLine.size() - 1; i++) {
        QPointF p1 = anodeLoadLine[i];
        QPointF p2 = anodeLoadLine[i + 1];

        for (int j = 0; j < cathodeLoadLine.size() - 1; j++) {
            QPointF p3 = cathodeLoadLine[j];
            QPointF p4 = cathodeLoadLine[j + 1];

            QPointF ip = lineIntersection(p1, p2, p3, p4);
            if (isOnSegment(ip, p1, p2) && isOnSegment(ip, p3, p4)) {
                return ip;
            }
        }
    }

    return intersection;
}

QPointF TriodeCommonCathode::lineIntersection(QPointF p1, QPointF p2, QPointF p3, QPointF p4)
{
    double x1 = p1.x(), y1 = p1.y();
    double x2 = p2.x(), y2 = p2.y();
    double x3 = p3.x(), y3 = p3.y();
    double x4 = p4.x(), y4 = p4.y();

    double a1 = y2 - y1;
    double b1 = x1 - x2;
    double c1 = a1 * x1 + b1 * y1;

    double a2 = y4 - y3;
    double b2 = x3 - x4;
    double c2 = a2 * x3 + b2 * y3;

    double d = a1 * b2 - a2 * b1;

    if (qAbs(d) < 1e-10) {
        return QPointF(0, 0);  // Parallel lines
    }

    double x = (b2 * c1 - b1 * c2) / d;
    double y = (a1 * c2 - a2 * c1) / d;

    return QPointF(x, y);
}

void TriodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Map the first parameter (Supply Voltage) to UI
    if (parameter[0]) {
        labels[0]->setVisible(true);
        values[0]->setVisible(true);
        labels[0]->setText(parameter[0]->getName());
        values[0]->setText(QString::number(parameter[0]->getValue(), 'f', 2));
    }

    // Map the second parameter (Cathode resistor Rk) to UI
    if (parameter[1]) {
        labels[1]->setVisible(true);
        values[1]->setVisible(true);
        labels[1]->setText(parameter[1]->getName());
        values[1]->setText(QString::number(parameter[1]->getValue(), 'f', 2));
    }

    // Map the third parameter (Anode resistor Ra) to UI
    if (parameter[2]) {
        labels[2]->setVisible(true);
        values[2]->setVisible(true);
        labels[2]->setText(parameter[2]->getName());
        values[2]->setText(QString::number(parameter[2]->getValue(), 'f', 2));
    }

    // Map the fourth parameter (Load impedance Rl) to UI
    if (parameter[3]) {
        labels[3]->setVisible(true);
        values[3]->setVisible(true);
        labels[3]->setText(parameter[3]->getName());
        values[3]->setText(QString::number(parameter[3]->getValue(), 'f', 2));
    }
}

bool TriodeCommonCathode::isOnSegment(QPointF p, QPointF a, QPointF b)
{
    return (p.x() >= qMin(a.x(), b.x()) && p.x() <= qMax(a.x(), b.x()) &&
            p.y() >= qMin(a.y(), b.y()) && p.y() <= qMax(a.y(), b.y()));
}

double TriodeCommonCathode::calculateGain()
{
    if (!device1 || operatingPoint.isNull()) return 0.0;

    double vb = getParameter(0);
    double ra = getParameter(1) * 1000;
    double rk = getParameter(2) * 1000;
    double rl = getParameter(3) * 1000;

    double va = operatingPoint.x();
    double ia = operatingPoint.y() / 1000;  // Convert to A
    double vk = ia * rk / 1000;  // Cathode voltage

    // Calculate transconductance (gm) using small signal analysis
    double deltaVa = 10.0;  // Small voltage change
    double ia1 = device1->anodeCurrent(vb, -vk);
    double ia2 = device1->anodeCurrent(vb - deltaVa, -vk);
    double gm = (ia1 - ia2) / deltaVa;

    // Gain calculation for unbypassed cathode
    double mu = device1->getParameter(0);  // Amplification factor (mu)
    double re = ra * rl / (ra + rl);  // Equivalent load resistance
    double gain = mu * re / (re + (mu + 1) * rk / 1000);  // Convert rk to kΩ

    return gain;
}
