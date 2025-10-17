#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "valvemodel/model/model.h"
#include "analyser/analyser.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{    
    ui->setupUi(this);

    QList<QSerialPortInfo> serialPorts = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &serialPort : serialPorts) {
        if (serialPort.vendorIdentifier() == 0x1a86 && serialPort.productIdentifier() == 0x7523) {
            ui->portSelect->addItem(serialPort.portName());
        }
    }

    ui->pentodeFit->addItem("Gardiner", GARDINER_PENTODE);
    ui->pentodeFit->addItem("Reefman (Derk)", REEFMAN_DERK_PENTODE);
    ui->pentodeFit->addItem("Reefman (DerkE)", REEFMAN_DERK_E_PENTODE);

    ui->sampling->addItem("Simple", SMP_LINEAR);
    ui->sampling->addItem("Optimised", SMP_LOGARITHMIC);

    // Create calibration controls
    QVBoxLayout *layout = new QVBoxLayout(this);

    heaterVoltageLabel = new QLabel("Heater Voltage Calibration:", this);
    heaterVoltageSpinBox = new QDoubleSpinBox(this);
    heaterVoltageSpinBox->setRange(0.0, 10.0);
    heaterVoltageSpinBox->setValue(1.0);
    layout->addWidget(heaterVoltageLabel);
    layout->addWidget(heaterVoltageSpinBox);

    heaterCurrentLabel = new QLabel("Heater Current Calibration:", this);
    heaterCurrentSpinBox = new QDoubleSpinBox(this);
    heaterCurrentSpinBox->setRange(0.0, 10.0);
    heaterCurrentSpinBox->setValue(1.0);
    layout->addWidget(heaterCurrentLabel);
    layout->addWidget(heaterCurrentSpinBox);

    anodeVoltageLabel = new QLabel("Anode Voltage Calibration:", this);
    anodeVoltageSpinBox = new QDoubleSpinBox(this);
    anodeVoltageSpinBox->setRange(0.0, 10.0);
    anodeVoltageSpinBox->setValue(1.0);
    layout->addWidget(anodeVoltageLabel);
    layout->addWidget(anodeVoltageSpinBox);

    anodeCurrentLabel = new QLabel("Anode Current Calibration:", this);
    anodeCurrentSpinBox = new QDoubleSpinBox(this);
    anodeCurrentSpinBox->setRange(0.0, 10.0);
    anodeCurrentSpinBox->setValue(1.0);
    layout->addWidget(anodeCurrentLabel);
    layout->addWidget(anodeCurrentSpinBox);

    screenVoltageLabel = new QLabel("Screen Voltage Calibration:", this);
    screenVoltageSpinBox = new QDoubleSpinBox(this);
    screenVoltageSpinBox->setRange(0.0, 10.0);
    screenVoltageSpinBox->setValue(1.0);
    layout->addWidget(screenVoltageLabel);
    layout->addWidget(screenVoltageSpinBox);

    screenCurrentLabel = new QLabel("Screen Current Calibration:", this);
    screenCurrentSpinBox = new QDoubleSpinBox(this);
    screenCurrentSpinBox->setRange(0.0, 10.0);
    screenCurrentSpinBox->setValue(1.0);
    layout->addWidget(screenCurrentLabel);
    layout->addWidget(screenCurrentSpinBox);

    gridVoltageLabel = new QLabel("Grid Voltage Calibration:", this);
    gridVoltageSpinBox = new QDoubleSpinBox(this);
    gridVoltageSpinBox->setRange(0.0, 10.0);
    gridVoltageSpinBox->setValue(1.0);
    layout->addWidget(gridVoltageLabel);
    layout->addWidget(gridVoltageSpinBox);

    gridCurrentLabel = new QLabel("Grid Current Calibration:", this);
    gridCurrentSpinBox = new QDoubleSpinBox(this);
    gridCurrentSpinBox->setRange(0.0, 10.0);
    gridCurrentSpinBox->setValue(1.0);
    layout->addWidget(gridCurrentLabel);
    layout->addWidget(gridCurrentSpinBox);

    ui->verticalLayout->addLayout(layout);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::setPort(QString port)
{
    for (int i = 0; i < ui->portSelect->count(); i++) {
        if (ui->portSelect->itemText(i) == port) {
            ui->portSelect->setCurrentIndex(i);
        }
    }
}

QString PreferencesDialog::getPort()
{
    return ui->portSelect->currentText();
}

int PreferencesDialog::getPentodeModelType()
{
    return ui->pentodeFit->currentData().toInt();
}

int PreferencesDialog::getSamplingType()
{
    return ui->sampling->currentData().toInt();
}

bool PreferencesDialog::useRemodelling()
{
    return ui->checkRemodel->isChecked();
}

bool PreferencesDialog::useSecondaryEmission()
{
    return ui->checkSecondary->isChecked();
}

bool PreferencesDialog::fixSecondaryEmission()
{
    return ui->checkFixSecondary->isChecked();
}

bool PreferencesDialog::fixTriodeParameters()
{
    return ui->checkFixTriode->isChecked();
}

bool PreferencesDialog::showScreenCurrent()
{
    return ui->checkScreenCurrent->isChecked();
}

double PreferencesDialog::getHeaterVoltageCalibration()
{
    return heaterVoltageSpinBox->value();
}

double PreferencesDialog::getHeaterCurrentCalibration()
{
    return heaterCurrentSpinBox->value();
}

double PreferencesDialog::getAnodeVoltageCalibration()
{
    return anodeVoltageSpinBox->value();
}

double PreferencesDialog::getAnodeCurrentCalibration()
{
    return anodeCurrentSpinBox->value();
}

double PreferencesDialog::getScreenVoltageCalibration()
{
    return screenVoltageSpinBox->value();
}

double PreferencesDialog::getScreenCurrentCalibration()
{
    return screenCurrentSpinBox->value();
}

double PreferencesDialog::getGridVoltageCalibration()
{
    return gridVoltageSpinBox->value();
}

double PreferencesDialog::getGridCurrentCalibration()
{
    return gridCurrentSpinBox->value();
}