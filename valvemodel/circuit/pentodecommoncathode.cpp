#include "pentodecommoncathode.h"

PentodeCommonCathode::PentodeCommonCathode()
{
    parameter[PENT_CC_VB] = new Parameter("Supply Voltage:", 300.0);
    parameter[PENT_CC_RK] = new Parameter("Cathode Resistor:", 1000.0);
    parameter[PENT_CC_RA] = new Parameter("Anode Resistor:", 100000.0);
    parameter[PENT_CC_VK] = new Parameter("Bias Point (Vk):", 0.0);
    parameter[PENT_CC_VA] = new Parameter("Anode Voltage:", 0.0);
    parameter[PENT_CC_IA] = new Parameter("Anode Current:", 0.0);
<<<<<<< Updated upstream
    parameter[PENT_CC_VG2] = new Parameter("Screen Voltage:", 0.0);
    parameter[PENT_CC_IG2] = new Parameter("Screen Current:", 0.0);
=======
>>>>>>> Stashed changes
    parameter[PENT_CC_RL] = new Parameter("Load Resistance:", 1000000.0);
    parameter[PENT_CC_AR] = new Parameter("Internal Anode Resistance:", 0.0);
    parameter[PENT_CC_GAIN] = new Parameter("Gain:", 0.0);
    parameter[PENT_CC_GAIN_B] = new Parameter("Gain (Rk bypassed):", 0.0);
<<<<<<< Updated upstream
=======
    parameter[PENT_CC_VG2] = new Parameter("Screen Grid Voltage:", 300.0);
    parameter[PENT_CC_RG2] = new Parameter("Screen Grid Resistor:", 47000.0);
>>>>>>> Stashed changes
}

void PentodeCommonCathode::updateUI(QLabel *labels[], QLineEdit *values[])
{
<<<<<<< Updated upstream
    for (int i = 0; i <= PENT_CC_GAIN_B; i++) {
        updateParameter(labels[i], values[i], parameter[i]);
    }

    for (int i = PENT_CC_GAIN_B + 1; i < 16; i++) {
=======
    for (int i = 0; i <= PENT_CC_RG2; i++) {
        updateParameter(labels[i], values[i], parameter[i]);
    }

    for (int i = PENT_CC_RG2 + 1; i < 16; i++) {
>>>>>>> Stashed changes
        labels[i]->setVisible(false);
        values[i]->setVisible(false);
    }
}

void PentodeCommonCathode::plot(Plot *plot)
{
<<<<<<< Updated upstream
    // Stub implementation
    // This would normally contain the pentode circuit simulation code
    // For now, we're just providing a minimal implementation to allow compilation
    if (device1 == nullptr) {
        return;
    }
=======
    if (device1 == nullptr) {
        return;
    }

    double vb = parameter[PENT_CC_VB]->getValue();
    double ra = parameter[PENT_CC_RA]->getValue();
    double rk = parameter[PENT_CC_RK]->getValue();
    double rl = parameter[PENT_CC_RL]->getValue();
    double vg2 = parameter[PENT_CC_VG2]->getValue();
    double rg2 = parameter[PENT_CC_RG2]->getValue();

    // First find the operating point using ngspice

    spice("source models/cc-pentode.cir");

    // Set pentode model parameters
    // Note: These parameters would need to be adjusted for pentode models
    // This is a placeholder implementation based on triode parameters
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

    // Additional pentode parameters would be set here
    // For example:
    // command = QString {"alterparam kg2=%1"}.arg(device1->getParameter(PAR_KG2));
    // spice(command.toStdString().c_str());

    spice("reset");

    command = QString {"alter vb=%1"}.arg(vb);
    spice(command.toStdString().c_str());

    command = QString {"alter rk=%1"}.arg(rk);
    spice(command.toStdString().c_str());

    command = QString {"alter ra=%1"}.arg(ra);
    spice(command.toStdString().c_str());

    command = QString {"alter vg2=%1"}.arg(vg2);
    spice(command.toStdString().c_str());

    command = QString {"alter rg2=%1"}.arg(rg2);
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
        parameter[PENT_CC_VK]->setValue(vgBias);
    }

    myvec = ngGet_Vec_Info(const_cast<char *>("V(4)"));
    if (myvec && myvec->v_length > 0) {
        parameter[PENT_CC_VA]->setValue(myvec->v_realdata[0]);
    }

    myvec = ngGet_Vec_Info(const_cast<char *>("I(VM)"));
    if (myvec && myvec->v_length > 0) {
        parameter[PENT_CC_IA]->setValue(myvec->v_realdata[0] * 1000.0);
    }

    QList<QGraphicsItem *> all;
    QList<QGraphicsItem *> cll;

    // Now calculate the load line
    double va = parameter[PENT_CC_VA]->getValue();
    double ia = parameter[PENT_CC_IA]->getValue() / 1000.0;

    // Calculate the internal anode resistance
    double ra_int = 0.0;
    double gm = 0.0;
    double mu = 0.0;

    // Simulate a small change in grid voltage
    command = QString {"alter vg=%1"}.arg(vgBias - 0.1);
    spice(command.toStdString().c_str());

    spice("bg_run");

    if (!waitSimulationEnd()) {
        // The simulation did not complete inside 2s
        return;
    }

    double va2 = 0.0;
    double ia2 = 0.0;

    myvec = ngGet_Vec_Info(const_cast<char *>("V(4)"));
    if (myvec && myvec->v_length > 0) {
        va2 = myvec->v_realdata[0];
    }

    myvec = ngGet_Vec_Info(const_cast<char *>("I(VM)"));
    if (myvec && myvec->v_length > 0) {
        ia2 = myvec->v_realdata[0];
    }

    gm = (ia2 - ia) / (-0.1);
    ra_int = (va2 - va) / (ia2 - ia);
    mu = gm * ra_int;

    parameter[PENT_CC_AR]->setValue(ra_int);

    // Calculate the gain
    double gain = mu * rl / (ra_int + rl) * ra / (ra + rl);
    parameter[PENT_CC_GAIN]->setValue(gain);

    double gain_b = mu * rl / (ra_int + rl);
    parameter[PENT_CC_GAIN_B]->setValue(gain_b);

    // Draw the load line
    double x1 = va - ia * ra;
    double y1 = 0.0;
    double x2 = va;
    double y2 = ia;

    QGraphicsLineItem *line = new QGraphicsLineItem(x1, y1, x2, y2);
    line->setPen(QPen(Qt::red, 2));
    all.append(line);

    // Draw the load line with external load
    x1 = va;
    y1 = ia;
    x2 = va + ia * rl;
    y2 = 0.0;

    line = new QGraphicsLineItem(x1, y1, x2, y2);
    line->setPen(QPen(Qt::blue, 2));
    all.append(line);

    // Draw the operating point
    QGraphicsEllipseItem *point = new QGraphicsEllipseItem(va - 2, ia * 1000.0 - 2, 4, 4);
    point->setBrush(QBrush(Qt::red));
    all.append(point);

    plot->setItems(all, cll);
>>>>>>> Stashed changes
}

int PentodeCommonCathode::getDeviceType(int index)
{
<<<<<<< Updated upstream
    // Mark parameter as used to avoid warnings
    (void)index;
    
    // Return pentode device type
    return PENTODE;
=======
    if (index == 0) {
        return DEVICE_PENTODE;
    } else {
        return DEVICE_NONE;
    }
>>>>>>> Stashed changes
}

QTreeWidgetItem *PentodeCommonCathode::buildTree(QTreeWidgetItem *parent)
{
<<<<<<< Updated upstream
    // Mark parameter as used to avoid warnings
    (void)parent;
    
    // Stub implementation
=======
    // Placeholder implementation
>>>>>>> Stashed changes
    return nullptr;
}

void PentodeCommonCathode::update(int index)
{
<<<<<<< Updated upstream
    // Mark parameter as used to avoid warnings
    (void)index;
    
    // Stub implementation
    // This would normally update the circuit parameters based on the changed parameter
=======
    if (index == PENT_CC_VB || index == PENT_CC_RK || index == PENT_CC_RA || 
        index == PENT_CC_VG2 || index == PENT_CC_RG2) {
        plot(nullptr);
    }
>>>>>>> Stashed changes
}
