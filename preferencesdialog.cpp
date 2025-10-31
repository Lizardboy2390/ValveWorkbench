#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "valvemodel/model/model.h"
#include "analyser/analyser.h"

#include <QGridLayout>
#include <QLabel>
#include <cmath>

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

    auto createCalibrationSpin = [this]() {
        auto *spin = new QDoubleSpinBox(this);
        spin->setRange(-10.0, 10.0);
        spin->setDecimals(4);
        spin->setSingleStep(0.01);
        spin->setValue(0.0);
        return spin;
    };

    auto createGridMeasurementSpin = [this]() {
        auto *spin = new QDoubleSpinBox(this);
        spin->setRange(-200.0, 200.0);
        spin->setDecimals(4);
        spin->setSingleStep(0.01);
        spin->setValue(0.0);
        return spin;
    };

    anodeVoltageSpinBox = createCalibrationSpin();
    anodeCurrentSpinBox = createCalibrationSpin();
    screenVoltageSpinBox = createCalibrationSpin();
    screenCurrentSpinBox = createCalibrationSpin();
    grid1VoltageSpinBox = createCalibrationSpin();
    grid2VoltageSpinBox = createCalibrationSpin();

    QGroupBox *calibrationGroup = new QGroupBox(tr("Calibration offsets"), this);
    QFormLayout *calibrationLayout = new QFormLayout(calibrationGroup);
    calibrationLayout->addRow(tr("Anode Voltage"), anodeVoltageSpinBox);
    calibrationLayout->addRow(tr("Anode Current"), anodeCurrentSpinBox);
    calibrationLayout->addRow(tr("Screen Voltage"), screenVoltageSpinBox);
    calibrationLayout->addRow(tr("Screen Current"), screenCurrentSpinBox);
    calibrationLayout->addRow(tr("Grid 1 Voltage"), grid1VoltageSpinBox);
    calibrationLayout->addRow(tr("Grid 2 Voltage"), grid2VoltageSpinBox);

    auto *gridCalibrationGroup = new QGroupBox(tr("Grid calibration"), this);
    auto *gridCalibrationLayout = new QGridLayout(gridCalibrationGroup);
    gridCalibrationLayout->addWidget(new QLabel(tr("Reference"), this), 0, 0);
    gridCalibrationLayout->addWidget(new QLabel(tr("Grid 1 measured"), this), 0, 1);
    gridCalibrationLayout->addWidget(new QLabel(tr("Grid 2 measured"), this), 0, 2);

    gridCalibrationLayout->addWidget(new QLabel(tr("-5 V command"), this), 1, 0);
    grid1MeasuredLowSpinBox = createGridMeasurementSpin();
    grid1MeasuredLowSpinBox->setValue(-PreferencesDialog::GRID_CAL_LOW_REF);
    grid2MeasuredLowSpinBox = createGridMeasurementSpin();
    grid2MeasuredLowSpinBox->setValue(-PreferencesDialog::GRID_CAL_LOW_REF);
    gridCalibrationLayout->addWidget(grid1MeasuredLowSpinBox, 1, 1);
    gridCalibrationLayout->addWidget(grid2MeasuredLowSpinBox, 1, 2);

    gridCalibrationLayout->addWidget(new QLabel(tr("-60 V command"), this), 2, 0);
    grid1MeasuredHighSpinBox = createGridMeasurementSpin();
    grid1MeasuredHighSpinBox->setValue(-PreferencesDialog::GRID_CAL_HIGH_REF);
    grid2MeasuredHighSpinBox = createGridMeasurementSpin();
    grid2MeasuredHighSpinBox->setValue(-PreferencesDialog::GRID_CAL_HIGH_REF);
    gridCalibrationLayout->addWidget(grid1MeasuredHighSpinBox, 2, 1);
    gridCalibrationLayout->addWidget(grid2MeasuredHighSpinBox, 2, 2);

    QWidget *scrollContents = new QWidget(this);
    auto *scrollContentsLayout = new QVBoxLayout(scrollContents);
    scrollContentsLayout->setContentsMargins(0, 0, 0, 0);
    scrollContentsLayout->setSpacing(12);
    scrollContentsLayout->addWidget(ui->verticalLayoutWidget);
    scrollContentsLayout->addWidget(calibrationGroup);
    scrollContentsLayout->addWidget(gridCalibrationGroup);
    scrollContentsLayout->addStretch();

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollContents);

    auto *dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(12, 12, 12, 12);
    dialogLayout->setSpacing(12);
    dialogLayout->addWidget(scrollArea);
    dialogLayout->addWidget(ui->buttonBox);

    setLayout(dialogLayout);
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

double PreferencesDialog::getGrid1VoltageCalibration()
{
    return grid1VoltageSpinBox->value();
}

double PreferencesDialog::getGrid2VoltageCalibration()
{
    return grid2VoltageSpinBox->value();
}

double PreferencesDialog::grid1CommandForDesired(double desiredVoltage) const
{
    return gridCommandForDesired(desiredVoltage, grid1MeasuredLowSpinBox->value(), grid1MeasuredHighSpinBox->value());
}

double PreferencesDialog::grid2CommandForDesired(double desiredVoltage) const
{
    return gridCommandForDesired(desiredVoltage, grid2MeasuredLowSpinBox->value(), grid2MeasuredHighSpinBox->value());
}

double PreferencesDialog::gridCommandForDesired(double desiredVoltage, double measuredLow, double measuredHigh) const
{
    const double commandLow = -GRID_CAL_LOW_REF;
    const double commandHigh = -GRID_CAL_HIGH_REF;

    const double commandSpan = commandHigh - commandLow;
    const double measuredSpan = measuredHigh - measuredLow;

    if (std::fabs(commandSpan) < GRID_CAL_EPSILON || std::fabs(measuredSpan) < GRID_CAL_EPSILON) {
        return desiredVoltage;
    }

    const double slope = measuredSpan / commandSpan;
    if (std::fabs(slope) < GRID_CAL_EPSILON) {
        return desiredVoltage;
    }

    const double offset = measuredLow - slope * commandLow;
    return (desiredVoltage - offset) / slope;
}