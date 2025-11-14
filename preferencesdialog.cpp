#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "valvemodel/model/model.h"
#include "analyser/analyser.h"

#include <QGridLayout>
#include <QLabel>
#include <cmath>
#include <QSettings>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{    
    ui->setupUi(this);

    QList<QSerialPortInfo> serialPorts = QSerialPortInfo::availablePorts();

    // Populate with all available ports (do not restrict to specific VID/PID)
    for (const QSerialPortInfo &serialPort : serialPorts) {
        ui->portSelect->addItem(serialPort.portName());
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

    // Apply reference to both grids controls
    applyLowRefBothCheckBox = new QCheckBox(tr("Apply −5 V command to both grids"), this);
    applyHighRefBothCheckBox = new QCheckBox(tr("Apply −60 V command to both grids"), this);
    gridCalibrationLayout->addWidget(applyLowRefBothCheckBox, 3, 0, 1, 3);
    gridCalibrationLayout->addWidget(applyHighRefBothCheckBox, 4, 0, 1, 3);

    // Mutually exclusive behavior for the two checkboxes
    connect(applyLowRefBothCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        if (checked) {
            // Uncheck the other without re-triggering its handler
            QSignalBlocker b(applyHighRefBothCheckBox);
            applyHighRefBothCheckBox->setChecked(false);
            emit applyGridRefRequested(PreferencesDialog::GRID_CAL_LOW_REF, true);
        } else {
            // If neither is checked after this change, turn grids off
            if (!applyHighRefBothCheckBox->isChecked()) {
                emit applyGridRefRequested(0.0, false);
            }
        }
    });
    connect(applyHighRefBothCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        if (checked) {
            QSignalBlocker b(applyLowRefBothCheckBox);
            applyLowRefBothCheckBox->setChecked(false);
            emit applyGridRefRequested(PreferencesDialog::GRID_CAL_HIGH_REF, true);
        } else {
            if (!applyLowRefBothCheckBox->isChecked()) {
                emit applyGridRefRequested(0.0, false);
            }
        }
    });

    // When measured values change:
    // - Normalize to negative (user can type 5.005 and we store -5.005)
    // - If cleared or near zero, reset to default reference for that row
    // - If a reference checkbox is active, re-emit to immediately reapply with new calibration
    auto onMeasuredChanged = [this](QDoubleSpinBox *spin, double defaultRef){
        QObject::connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, spin, defaultRef](double v){
            // Treat near-zero or cleared as reset to default
            if (std::fabs(v) < GRID_CAL_EPSILON) {
                QSignalBlocker blocker(spin);
                spin->setValue(-defaultRef);
                v = -defaultRef;
            }
            // Normalize positive entries to negative
            if (v > 0.0) {
                QSignalBlocker blocker(spin);
                spin->setValue(-v);
                v = -v;
            }
            Q_UNUSED(v);
            if (applyLowRefBothCheckBox->isChecked()) {
                emit applyGridRefRequested(PreferencesDialog::GRID_CAL_LOW_REF, true);
            } else if (applyHighRefBothCheckBox->isChecked()) {
                emit applyGridRefRequested(PreferencesDialog::GRID_CAL_HIGH_REF, true);
            }
        });
    };

    onMeasuredChanged(grid1MeasuredLowSpinBox, PreferencesDialog::GRID_CAL_LOW_REF);
    onMeasuredChanged(grid1MeasuredHighSpinBox, PreferencesDialog::GRID_CAL_HIGH_REF);
    onMeasuredChanged(grid2MeasuredLowSpinBox, PreferencesDialog::GRID_CAL_LOW_REF);
    onMeasuredChanged(grid2MeasuredHighSpinBox, PreferencesDialog::GRID_CAL_HIGH_REF);

    // Load persisted settings (port, model options, calibration, measured references)
    loadFromSettings();

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

double PreferencesDialog::getGrid1MeasuredLow() const { return grid1MeasuredLowSpinBox->value(); }
double PreferencesDialog::getGrid1MeasuredHigh() const { return grid1MeasuredHighSpinBox->value(); }
double PreferencesDialog::getGrid2MeasuredLow() const { return grid2MeasuredLowSpinBox->value(); }
double PreferencesDialog::getGrid2MeasuredHigh() const { return grid2MeasuredHighSpinBox->value(); }

void PreferencesDialog::loadFromSettings()
{
    QSettings s("ValveWorkbench", "ValveWorkbench");

    // Port and model/sampling options
    QString savedPort = s.value("preferences/port", "").toString();
    if (!savedPort.isEmpty()) {
        // Ensure the saved port is present even if not currently detected
        bool found = false;
        for (int i = 0; i < ui->portSelect->count(); ++i) {
            if (ui->portSelect->itemText(i) == savedPort) { found = true; break; }
        }
        if (!found) {
            ui->portSelect->addItem(savedPort);
        }
        setPort(savedPort);
    }
    int savedPentodeFit = s.value("preferences/pentodeFit", GARDINER_PENTODE).toInt();
    int idxFit = ui->pentodeFit->findData(savedPentodeFit);
    if (idxFit >= 0) ui->pentodeFit->setCurrentIndex(idxFit);
    int savedSampling = s.value("preferences/sampling", SMP_LINEAR).toInt();
    int idxSamp = ui->sampling->findData(savedSampling);
    if (idxSamp >= 0) ui->sampling->setCurrentIndex(idxSamp);

    ui->checkScreenCurrent->setChecked(s.value("preferences/showScreenCurrent", true).toBool());
    ui->checkRemodel->setChecked(s.value("preferences/useRemodelling", false).toBool());
    ui->checkSecondary->setChecked(s.value("preferences/useSecondaryEmission", true).toBool());
    ui->checkFixTriode->setChecked(s.value("preferences/fixTriodeParameters", true).toBool());
    ui->checkFixSecondary->setChecked(s.value("preferences/fixSecondaryEmission", true).toBool());

    // Calibration offsets
    anodeVoltageSpinBox->setValue(s.value("cal/anodeVoltage", 0.0).toDouble());
    anodeCurrentSpinBox->setValue(s.value("cal/anodeCurrent", 0.0).toDouble());
    screenVoltageSpinBox->setValue(s.value("cal/screenVoltage", 0.0).toDouble());
    screenCurrentSpinBox->setValue(s.value("cal/screenCurrent", 0.0).toDouble());
    grid1VoltageSpinBox->setValue(s.value("cal/grid1Voltage", 0.0).toDouble());
    grid2VoltageSpinBox->setValue(s.value("cal/grid2Voltage", 0.0).toDouble());

    // Measured grid references
    grid1MeasuredLowSpinBox->setValue(s.value("gridCal/g1Low", -PreferencesDialog::GRID_CAL_LOW_REF).toDouble());
    grid1MeasuredHighSpinBox->setValue(s.value("gridCal/g1High", -PreferencesDialog::GRID_CAL_HIGH_REF).toDouble());
    grid2MeasuredLowSpinBox->setValue(s.value("gridCal/g2Low", -PreferencesDialog::GRID_CAL_LOW_REF).toDouble());
    grid2MeasuredHighSpinBox->setValue(s.value("gridCal/g2High", -PreferencesDialog::GRID_CAL_HIGH_REF).toDouble());
}

void PreferencesDialog::saveToSettings() const
{
    QSettings s("ValveWorkbench", "ValveWorkbench");

    s.setValue("preferences/port", ui->portSelect->currentText());
    s.setValue("preferences/pentodeFit", ui->pentodeFit->currentData().toInt());
    s.setValue("preferences/sampling", ui->sampling->currentData().toInt());
    s.setValue("preferences/showScreenCurrent", ui->checkScreenCurrent->isChecked());
    s.setValue("preferences/useRemodelling", ui->checkRemodel->isChecked());
    s.setValue("preferences/useSecondaryEmission", ui->checkSecondary->isChecked());
    s.setValue("preferences/fixTriodeParameters", ui->checkFixTriode->isChecked());
    s.setValue("preferences/fixSecondaryEmission", ui->checkFixSecondary->isChecked());

    s.setValue("cal/anodeVoltage", anodeVoltageSpinBox->value());
    s.setValue("cal/anodeCurrent", anodeCurrentSpinBox->value());
    s.setValue("cal/screenVoltage", screenVoltageSpinBox->value());
    s.setValue("cal/screenCurrent", screenCurrentSpinBox->value());
    s.setValue("cal/grid1Voltage", grid1VoltageSpinBox->value());
    s.setValue("cal/grid2Voltage", grid2VoltageSpinBox->value());

    s.setValue("gridCal/g1Low", grid1MeasuredLowSpinBox->value());
    s.setValue("gridCal/g1High", grid1MeasuredHighSpinBox->value());
    s.setValue("gridCal/g2Low", grid2MeasuredLowSpinBox->value());
    s.setValue("gridCal/g2High", grid2MeasuredHighSpinBox->value());
}