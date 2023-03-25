#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "valvemodel/model/model.h"

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

    ui->sampling->addItem("Linear", 0);
    ui->sampling->addItem("Logarithmic", 1);
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
