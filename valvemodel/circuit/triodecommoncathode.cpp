#include "triodecommoncathode.h"

TriodeCommonCathode::TriodeCommonCathode()
{
    parameter[TRI_CC_VB] = new Parameter("Supply Voltage:", 300.0);
    parameter[TRI_CC_RK] = new Parameter("Cathode Resistor:", 1000.0);
    parameter[TRI_CC_RA] = new Parameter("Anode Resistor:", 100000.0);
    parameter[TRI_CC_IA] = new Parameter("Anode Current:", 0.0);
    parameter[TRI_CC_VK] = new Parameter("Bias Point (Vk):", 0.0);
}

void TriodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    for (int i = 0; i <= TRI_CC_VK; i++) {
        updateParameter(labels[i], values[i], parameter[i]);
    }
}

void TriodeCommonCathode::plot(Plot *plot, Device *device)
{
    QList<QGraphicsItem *> all;
    QList<QGraphicsItem *> cll;

    if (anodeLoadLine != nullptr) {
        plot->getScene()->removeItem(anodeLoadLine);
    }

    if (cathodeLoadLine != nullptr) {
        plot->getScene()->removeItem(cathodeLoadLine);
    }

    QPen modelPen;
    modelPen.setColor(QColor::fromRgb(0, 0, 255));

    double vb = parameter[TRI_CC_VB]->getValue();
    double ra = parameter[TRI_CC_RA]->getValue();
    double rk = parameter[TRI_CC_RK]->getValue();

    double ia = vb * 1000.0 / ra;

    all.append(plot->createSegment(0.0, ia, vb, 0, modelPen));

    anodeLoadLine = plot->getScene()->createItemGroup(all);

    modelPen.setColor(QColor::fromRgb(0, 255, 0));

    double vaBias = 0.0;
    double vgBias = 0.0;
    double iaBias = 0.0;

    double vgMax = device->getVg1Max();

    double iaMinErr = device->getIaMax();
    double vg = vgMax / 1000.0;
    ia = vg * 1000.0 / rk;
    double va = device->anodeVoltage(ia, -vg);;
    for (int j = 2; j < 1001; j++) {
        vg = vgMax * j / 1000.0;
        double iaNext = vg * 1000.0 / rk;
        double vaNext = device->anodeVoltage(iaNext, -vg);
        cll.append(plot->createSegment(va, ia, vaNext, iaNext, modelPen));
        ia = iaNext;
        va = vaNext;

        double iaLoadLine = (vb - va) * 1000.0 / ra;
        double iaErr = abs(iaLoadLine - ia);
        if (iaErr < iaMinErr) {
            iaMinErr = iaErr;
            vaBias = va;
            vgBias = vg;
            iaBias = ia;
        }
    }

    parameter[TRI_CC_VK]->setValue(vgBias);
    parameter[TRI_CC_IA]->setValue(iaBias);

    cathodeLoadLine = plot->getScene()->createItemGroup(cll);
}

void TriodeCommonCathode::update(int index)
{
    switch (index) {
    case TRI_CC_VB:
        break;
    case TRI_CC_RK:
        break;
    case TRI_CC_RA:
        break;
    case TRI_CC_IA:
        break;
    case TRI_CC_VK:
        break;
    default:
        break;
    }
}
