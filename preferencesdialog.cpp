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

// Calibration getters
double PreferencesDialog::getHeaterVoltageCalibration() const
{
    return ui->heaterVoltageCalibration->value();
}

double PreferencesDialog::getAnodeVoltageCalibration() const
{
    return ui->anodeVoltageCalibration->value();
}

double PreferencesDialog::getScreenVoltageCalibration() const
{
    return ui->screenVoltageCalibration->value();
}

double PreferencesDialog::getGridVoltageCalibration() const
{
    return ui->gridVoltageCalibration->value();
}

double PreferencesDialog::getHeaterCurrentCalibration() const
{
    return ui->heaterCurrentCalibration->value();
}

double PreferencesDialog::getAnodeCurrentCalibration() const
{
    return ui->anodeCurrentCalibration->value();
}

double PreferencesDialog::getScreenCurrentCalibration() const
{
    return ui->screenCurrentCalibration->value();
}

// Calibration setters
void PreferencesDialog::setHeaterVoltageCalibration(double value)
{
    ui->heaterVoltageCalibration->setValue(value);
}

void PreferencesDialog::setAnodeVoltageCalibration(double value)
{
    ui->anodeVoltageCalibration->setValue(value);
}

void PreferencesDialog::setScreenVoltageCalibration(double value)
{
    ui->screenVoltageCalibration->setValue(value);
}

void PreferencesDialog::setGridVoltageCalibration(double value)
{
    ui->gridVoltageCalibration->setValue(value);
}

void PreferencesDialog::setHeaterCurrentCalibration(double value)
{
    ui->heaterCurrentCalibration->setValue(value);
}

void PreferencesDialog::setAnodeCurrentCalibration(double value)
{
    ui->anodeCurrentCalibration->setValue(value);
}

void PreferencesDialog::setScreenCurrentCalibration(double value)
{
    ui->screenCurrentCalibration->setValue(value);
}
