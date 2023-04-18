#include "triodecommoncathode.h"

TriodeCommonCathode::TriodeCommonCathode()
{
    parameter[TRI_CC_VB] = new Parameter("Supply Voltage:", 300.0);
    parameter[TRI_CC_RK] = new Parameter("Cathode Resistor:", 1000.0);
    parameter[TRI_CC_RA] = new Parameter("Anode Resistor:", 100000.0);
    parameter[TRI_CC_VK] = new Parameter("Bias Point (Vk):", 0.0);
    parameter[TRI_CC_VA] = new Parameter("Anode Voltage:", 0.0);
    parameter[TRI_CC_IA] = new Parameter("Anode Current:", 0.0);
    parameter[TRI_CC_RL] = new Parameter("Load Resistance:", 1000000.0);
    parameter[TRI_CC_AR] = new Parameter("Internal Anode Resistance:", 0.0);
    parameter[TRI_CC_GAIN] = new Parameter("Gain:", 0.0);
    parameter[TRI_CC_GAIN_B] = new Parameter("Gain (Rk bypassed):", 0.0);
}

void TriodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    for (int i = 0; i <= TRI_CC_GAIN_B; i++) {
        updateParameter(labels[i], values[i], parameter[i]);
    }

    for (int i = TRI_CC_GAIN_B + 1; i < 16; i++) {
        labels[i]->setVisible(false);
        values[i]->setVisible(false);
    }
}

void TriodeCommonCathode::plot(Plot *plot)
{
    if (device1 == nullptr) {
        return;
    }

    double vb = parameter[TRI_CC_VB]->getValue();
    double ra = parameter[TRI_CC_RA]->getValue();
    double rk = parameter[TRI_CC_RK]->getValue();
    double rl = parameter[TRI_CC_RL]->getValue();

    // First find the operating point using ngspice

    spice("source models/cc-triode.cir");

    QString command = QString {"alterparam mu=%1"}.arg(device1->getParameter(PAR_MU));
    spice(command.toStdString().c_str());

    command = QString {"alterparam kg1=%1"}.arg(device1->getParameter(PAR_KG1) * 1000.0);
    spice(command.toStdString().c_str());

    command = QString {"alterparam x=%1"}.arg(device1->getParameter(PAR_X));
    spice(command.toStdString().c_str());

    command = QString {"alterparam kp=%1"}.arg(device1->getParameter(PAR_KP));
    spice(command.toStdString().c_str());

    command = QString {"alterparam kvb=%1"}.arg(device1->getParameter(PAR_KVB));
    spice(command.toStdString().c_str());

    command = QString {"alterparam kvb1=%1"}.arg(device1->getParameter(PAR_KVB1));
    spice(command.toStdString().c_str());

    command = QString {"alterparam vct=%1"}.arg(device1->getParameter(PAR_VCT));
    spice(command.toStdString().c_str());

    spice("reset");

    command = QString {"alter vb=%1"}.arg(vb);
    spice(command.toStdString().c_str());

    command = QString {"alter rk=%1"}.arg(rk);
    spice(command.toStdString().c_str());

    command = QString {"alter ra=%1"}.arg(ra);
    spice(command.toStdString().c_str());

    spice("bg_run");

    if (!waitSimulationEnd()) {
        // The simulation did not complete inside 2s
        return;
    }

    double vgBias = 0.0;
    pvector_info myvec = ngGet_Vec_Info(const_cast<char *>("V(3)"));
    if (myvec && myvec->v_length > 0) {
        vgBias = myvec->v_realdata[0];
        parameter[TRI_CC_VK]->setValue(vgBias);
    }

    myvec = ngGet_Vec_Info(const_cast<char *>("V(4)"));
    if (myvec && myvec->v_length > 0) {
        parameter[TRI_CC_VA]->setValue(myvec->v_realdata[0]);
    }

    myvec = ngGet_Vec_Info(const_cast<char *>("I(VM)"));
    if (myvec && myvec->v_length > 0) {
        parameter[TRI_CC_IA]->setValue(myvec->v_realdata[0] * 1000.0);
    }
    QList<QGraphicsItem *> all;
    QList<QGraphicsItem *> cll;

    /*if (anodeLoadLine != nullptr) {
        plot->getScene()->removeItem(anodeLoadLine);
    }

    if (cathodeLoadLine != nullptr) {
        plot->getScene()->removeItem(cathodeLoadLine);
    }*/

    QPen modelPen;
    modelPen.setColor(QColor::fromRgb(0, 0, 255));

    double ia = vb * 1000.0 / (ra + rk);

    all.append(plot->createSegment(0.0, ia, vb, 0, modelPen));

    anodeLoadLine = plot->getScene()->createItemGroup(all);

    modelPen.setColor(QColor::fromRgb(0, 255, 0));

    double vgMax = device1->getVg1Max();

    double vg = vgMax / 1000.0;
    ia = vg * 1000.0 / rk;
    double va = device1->anodeVoltage(ia, -vg);;
    for (int j = 2; j < 1001 && vb > va; j++) {
        vg = vgMax * j / 1000.0;
        double iaNext = vg * 1000.0 / rk;
        double vaNext = device1->anodeVoltage(iaNext, -vg);

        cll.append(plot->createSegment(va, ia, vaNext, iaNext, modelPen));
        ia = iaNext;
        va = vaNext;
    }

    double ia1 = device1->anodeCurrent(vb, -vgBias);
    double ia2 = device1->anodeCurrent(vb - 10.0, -vgBias);

    double ar = 10000.0 / (ia1 - ia2);
    parameter[TRI_CC_AR]->setValue(ar);

    double mu = device1->getParameter(PAR_MU);

    double re = ra * rl / (ra + rl);

    double ark = ar + rk * (mu + 1.0);
    parameter[TRI_CC_GAIN]->setValue(mu * re / (re + ark));
    parameter[TRI_CC_GAIN_B]->setValue(mu * re / (re + ar));

    cathodeLoadLine = plot->getScene()->createItemGroup(cll);
}

int TriodeCommonCathode::getDeviceType(int index)
{
    if (index == 1) {
        return MODEL_TRIODE;
    }

    return -1;
}

QTreeWidgetItem *TriodeCommonCathode::buildTree(QTreeWidgetItem *parent)
{
    return nullptr;
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
    case TRI_CC_RL:
        break;
    case TRI_CC_VA:
        break;
    case TRI_CC_IA:
        break;
    case TRI_CC_VK:
        break;
    case TRI_CC_GAIN:
        break;
    default:
        break;
    }
}
