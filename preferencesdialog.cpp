#include "preferencesdialog.h"
#include <ui_preferencesdialog.h>
#include "valvemodel/model/model.h"
#include "analyser/analyser.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QSignalBlocker>
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
    ui->pentodeFit->addItem("ExtractModel (DerkE exact)", EXTRACT_DERK_E_PENTODE);
    ui->pentodeFit->addItem("Simple Manual Pentode", SIMPLE_MANUAL_PENTODE);

    ui->sampling->addItem("Linear", SMP_LINEAR);
    ui->sampling->addItem("Logarithmic", SMP_LOGARITHMIC);

    ui->avgMode->addItem("Auto");
    ui->avgMode->addItem("Fixed");
    ui->avgMode->setCurrentIndex(0);

    ui->avgSamples->setMinimum(1);
    ui->avgSamples->setMaximum(8);
    ui->avgSamples->setValue(5);

    auto createGridMeasurementSpin = [this]() {
        auto *spin = new QDoubleSpinBox(this);
        spin->setRange(-200.0, 200.0);
        spin->setDecimals(4);
        spin->setSingleStep(0.01);
        spin->setValue(0.0);
        return spin;
    };

    calibrationLoadResistorSpinBox = new QDoubleSpinBox(this);
    calibrationLoadResistorSpinBox->setRange(1000.0, 1000000.0);
    calibrationLoadResistorSpinBox->setDecimals(0);
    calibrationLoadResistorSpinBox->setSingleStep(1000.0);
    calibrationLoadResistorSpinBox->setValue(47000.0);
    calibrationLoadResistorSpinBox->setSuffix(tr(" \u03a9"));

    calibrationLoadResistorWattsSpinBox = new QDoubleSpinBox(this);
    calibrationLoadResistorWattsSpinBox->setRange(0.1, 50.0);
    calibrationLoadResistorWattsSpinBox->setDecimals(2);
    calibrationLoadResistorWattsSpinBox->setSingleStep(0.5);
    calibrationLoadResistorWattsSpinBox->setValue(2.0);
    calibrationLoadResistorWattsSpinBox->setSuffix(tr(" W"));

    linkHvVoltagePointsCheckBox = new QCheckBox(tr("Link HV1/HV2 voltage points"), this);
    linkHvVoltagePointsCheckBox->setChecked(true);

    hvSampleOnceButton = new QPushButton(tr("Sample once (M2)"), this);
    hvStopButton = new QPushButton(tr("Stop HV"), this);
    hvDischargeButton = new QPushButton(tr("Discharge (M1)"), this);
    hvCalRawLabel = new QLabel(tr("No HV sample"), this);

    QGroupBox *calibrationGroup = new QGroupBox(tr("HV calibration"), this);
    auto *calibrationGroupLayout = new QVBoxLayout(calibrationGroup);
    calibrationGroupLayout->setContentsMargins(8, 8, 8, 8);
    calibrationGroupLayout->setSpacing(10);

    QFormLayout *calibrationLayout = new QFormLayout();
    calibrationLayout->addRow(tr("Calibration load resistor"), calibrationLoadResistorSpinBox);
    calibrationLayout->addRow(tr("Load resistor power"), calibrationLoadResistorWattsSpinBox);
    calibrationLayout->addRow(QString(), linkHvVoltagePointsCheckBox);
    calibrationLayout->addRow(tr("HV calibration"), hvSampleOnceButton);

    {
        auto *hvButtons = new QWidget(this);
        auto *hvButtonsLayout = new QHBoxLayout(hvButtons);
        hvButtonsLayout->setContentsMargins(0, 0, 0, 0);
        hvButtonsLayout->addWidget(hvStopButton);
        hvButtonsLayout->addWidget(hvDischargeButton);
        calibrationLayout->addRow(tr("HV control"), hvButtons);
    }
    calibrationLayout->addRow(tr("HV raw ADC"), hvCalRawLabel);
    calibrationGroupLayout->addLayout(calibrationLayout);

    connect(hvSampleOnceButton, &QPushButton::clicked, this, &PreferencesDialog::requestHvCalibrationSampleOnce);
    connect(hvDischargeButton, &QPushButton::clicked, this, &PreferencesDialog::hvDischargeRequested);
    connect(hvStopButton, &QPushButton::clicked, this, [this](){
        if (hv1HoldActiveButton) {
            hv1HoldActiveButton->setText(tr("Hold"));
            hv1HoldActiveButton = nullptr;
        }
        if (hv2HoldActiveButton) {
            hv2HoldActiveButton->setText(tr("Hold"));
            hv2HoldActiveButton = nullptr;
        }
        emit hvHoldRequested(0, 0.0, false);
    });

    auto createAdcSpin = [this]() {
        auto *spin = new QSpinBox(this);
        spin->setRange(0, 4095);
        spin->setSingleStep(1);
        spin->setValue(0);
        return spin;
    };

    auto createHvPointVoltsSpin = [this]() {
        auto *spin = new QDoubleSpinBox(this);
        spin->setRange(0.0, 600.0);
        spin->setDecimals(3);
        spin->setSingleStep(1.0);
        spin->setValue(0.0);
        spin->setSuffix(tr(" V"));
        return spin;
    };

    auto addHvPointTable = [this, &createHvPointVoltsSpin, &createAdcSpin](QVBoxLayout *parentLayout,
                                                                          const QString &title,
                                                                          bool isHv1) {
        auto *group = new QGroupBox(title, this);
        auto *grid = new QGridLayout(group);

        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(2, 0);
        grid->setColumnStretch(3, 0);
        grid->setColumnStretch(4, 0);
        grid->setColumnStretch(5, 0);
        grid->setColumnStretch(6, 0);

        grid->addWidget(new QLabel(tr("Point"), this), 0, 0);
        grid->addWidget(new QLabel(tr("Measured V"), this), 0, 1);
        grid->addWidget(new QLabel(tr("HV ADC"), this), 0, 2);
        grid->addWidget(new QLabel(tr("IA_HI ADC"), this), 0, 3);
        grid->addWidget(new QLabel(tr("IA_LO ADC"), this), 0, 4);
        grid->addWidget(new QLabel(tr("Capture"), this), 0, 5);
        grid->addWidget(new QLabel(tr("Hold"), this), 0, 6);

        for (int i = 0; i < 6; ++i) {
            auto *lbl = new QLabel(QString::number(i + 1), this);
            auto *vSpin = createHvPointVoltsSpin();
            auto *hvSpin = createAdcSpin();
            auto *iaHiSpin = createAdcSpin();
            auto *iaLoSpin = createAdcSpin();
            auto *cap = new QPushButton(tr("Use last"), this);
            auto *hold = new QPushButton(tr("Hold"), this);

            QObject::connect(cap, &QPushButton::clicked, this, [this, isHv1, hvSpin, iaHiSpin, iaLoSpin](){
                if (isHv1) {
                    hvSpin->setValue(lastHv1Adc);
                    iaHiSpin->setValue(lastIaHi1Adc);
                    iaLoSpin->setValue(lastIaLo1Adc);
                } else {
                    hvSpin->setValue(lastHv2Adc);
                    iaHiSpin->setValue(lastIaHi2Adc);
                    iaLoSpin->setValue(lastIaLo2Adc);
                }
            });

            QObject::connect(hold, &QPushButton::clicked, this, [this, isHv1, vSpin, hold](){
                const int channel = isHv1 ? 1 : 2;
                QPushButton *&active = isHv1 ? hv1HoldActiveButton : hv2HoldActiveButton;
                if (active == hold) {
                    hold->setText(tr("Hold"));
                    active = nullptr;
                    emit hvHoldRequested(channel, 0.0, false);
                    return;
                }
                if (active) {
                    active->setText(tr("Hold"));
                    emit hvHoldRequested(channel, 0.0, false);
                }
                active = hold;
                hold->setText(tr("Holding"));
                emit hvHoldRequested(channel, vSpin->value(), true);
            });

            if (isHv1) {
                hv1PointVoltsSpin.push_back(vSpin);
                hv1PointHvAdcSpin.push_back(hvSpin);
                hv1PointIaHiAdcSpin.push_back(iaHiSpin);
                hv1PointIaLoAdcSpin.push_back(iaLoSpin);
            } else {
                hv2PointVoltsSpin.push_back(vSpin);
                hv2PointHvAdcSpin.push_back(hvSpin);
                hv2PointIaHiAdcSpin.push_back(iaHiSpin);
                hv2PointIaLoAdcSpin.push_back(iaLoSpin);
            }

            const int row = i + 1;
            grid->addWidget(lbl, row, 0);
            grid->addWidget(vSpin, row, 1);
            grid->addWidget(hvSpin, row, 2);
            grid->addWidget(iaHiSpin, row, 3);
            grid->addWidget(iaLoSpin, row, 4);
            grid->addWidget(cap, row, 5);
            grid->addWidget(hold, row, 6);
        }

        parentLayout->addWidget(group);
    };

    addHvPointTable(calibrationGroupLayout, tr("HV1 (Anode 1) points"), true);
    addHvPointTable(calibrationGroupLayout, tr("HV2 (Screen) points"), false);

    auto syncLinkedHvVoltsIfSafe = [this]() {
        if (!linkHvVoltagePointsCheckBox || !linkHvVoltagePointsCheckBox->isChecked()) {
            return;
        }

        auto allAdcZero = [](const QVector<QSpinBox*> &xs) {
            for (int i = 0; i < xs.size() && i < 6; ++i) {
                if (xs[i]->value() != 0) return false;
            }
            return true;
        };

        const bool hv1Uncal = allAdcZero(hv1PointHvAdcSpin) && allAdcZero(hv1PointIaHiAdcSpin) && allAdcZero(hv1PointIaLoAdcSpin);
        const bool hv2Uncal = allAdcZero(hv2PointHvAdcSpin) && allAdcZero(hv2PointIaHiAdcSpin) && allAdcZero(hv2PointIaLoAdcSpin);

        if (!(hv1Uncal && hv2Uncal)) {
            return;
        }

        for (int i = 0; i < hv1PointVoltsSpin.size() && i < hv2PointVoltsSpin.size() && i < 6; ++i) {
            QSignalBlocker b(hv2PointVoltsSpin[i]);
            hv2PointVoltsSpin[i]->setValue(hv1PointVoltsSpin[i]->value());
        }
        if (hv1LastSuggestedVolts.size() >= 6) {
            hv2LastSuggestedVolts = hv1LastSuggestedVolts;
        }
    };

    for (int i = 0; i < hv1PointVoltsSpin.size() && i < hv2PointVoltsSpin.size() && i < 6; ++i) {
        QObject::connect(hv1PointVoltsSpin[i], qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, i](double v){
            if (!linkHvVoltagePointsCheckBox || !linkHvVoltagePointsCheckBox->isChecked()) return;
            if (i >= hv2PointVoltsSpin.size()) return;
            if (std::fabs(hv2PointVoltsSpin[i]->value() - v) < 1e-9) return;
            QSignalBlocker b(hv2PointVoltsSpin[i]);
            hv2PointVoltsSpin[i]->setValue(v);
        });
        QObject::connect(hv2PointVoltsSpin[i], qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, i](double v){
            if (!linkHvVoltagePointsCheckBox || !linkHvVoltagePointsCheckBox->isChecked()) return;
            if (i >= hv1PointVoltsSpin.size()) return;
            if (std::fabs(hv1PointVoltsSpin[i]->value() - v) < 1e-9) return;
            QSignalBlocker b(hv1PointVoltsSpin[i]);
            hv1PointVoltsSpin[i]->setValue(v);
        });
    }

    QObject::connect(linkHvVoltagePointsCheckBox, &QCheckBox::toggled, this, [syncLinkedHvVoltsIfSafe](bool){
        syncLinkedHvVoltsIfSafe();
    });

    auto updateSuggestedHvVolts = [this]() {
        const double rOhms = calibrationLoadResistorSpinBox ? calibrationLoadResistorSpinBox->value() : 47000.0;
        const double watts = calibrationLoadResistorWattsSpinBox ? calibrationLoadResistorWattsSpinBox->value() : 2.0;

        if (!(rOhms > 1.0) || !(watts > 0.0)) {
            return;
        }

        double vmax = std::sqrt(rOhms * watts);
        if (vmax > 600.0) vmax = 600.0;

        const QVector<double> fractions = {0.25, 0.50, 0.75, 0.85, 0.925, 1.00};
        QVector<double> suggested;
        suggested.reserve(6);
        for (int i = 0; i < fractions.size(); ++i) {
            double v = vmax * fractions.at(i);
            if (v < 0.0) v = 0.0;
            if (v > 600.0) v = 600.0;
            v = std::round(v);
            suggested.push_back(v);
        }

        auto shouldAutoUpdate = [](const QVector<QDoubleSpinBox*> &vSpins,
                                   const QVector<QSpinBox*> &hvAdc,
                                   const QVector<QSpinBox*> &iaHi,
                                   const QVector<QSpinBox*> &iaLo,
                                   const QVector<double> &lastSuggested) {
            for (int i = 0; i < hvAdc.size() && i < 6; ++i) {
                if (hvAdc[i]->value() != 0) return false;
            }
            for (int i = 0; i < iaHi.size() && i < 6; ++i) {
                if (iaHi[i]->value() != 0) return false;
            }
            for (int i = 0; i < iaLo.size() && i < 6; ++i) {
                if (iaLo[i]->value() != 0) return false;
            }

            if (lastSuggested.size() >= 6) {
                for (int i = 0; i < vSpins.size() && i < 6; ++i) {
                    if (std::fabs(vSpins[i]->value() - lastSuggested.at(i)) > 1e-6) return false;
                }
                return true;
            }

            for (int i = 0; i < vSpins.size() && i < 6; ++i) {
                if (std::fabs(vSpins[i]->value()) > 1e-6) return false;
            }
            return true;
        };

        if (shouldAutoUpdate(hv1PointVoltsSpin, hv1PointHvAdcSpin, hv1PointIaHiAdcSpin, hv1PointIaLoAdcSpin, hv1LastSuggestedVolts)) {
            hv1LastSuggestedVolts = suggested;
            for (int i = 0; i < hv1PointVoltsSpin.size() && i < suggested.size() && i < 6; ++i) {
                hv1PointVoltsSpin[i]->setValue(suggested.at(i));
            }
        }
        if (shouldAutoUpdate(hv2PointVoltsSpin, hv2PointHvAdcSpin, hv2PointIaHiAdcSpin, hv2PointIaLoAdcSpin, hv2LastSuggestedVolts)) {
            hv2LastSuggestedVolts = suggested;
            for (int i = 0; i < hv2PointVoltsSpin.size() && i < suggested.size() && i < 6; ++i) {
                hv2PointVoltsSpin[i]->setValue(suggested.at(i));
            }
        }
    };

    QObject::connect(calibrationLoadResistorSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateSuggestedHvVolts](double){
        updateSuggestedHvVolts();
    });
    QObject::connect(calibrationLoadResistorWattsSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [updateSuggestedHvVolts](double){
        updateSuggestedHvVolts();
    });
    updateSuggestedHvVolts();

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
            if (std::fabs(v) < GRID_CAL_EPSILON) {
                QSignalBlocker blocker(spin);
                spin->setValue(-defaultRef);
                v = -defaultRef;
            }
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

    setMinimumSize(1100, 900);
    resize(1200, 950);
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

int PreferencesDialog::getAveragingMode()
{
    return ui->avgMode->currentIndex();
}

int PreferencesDialog::getAveragingFixedSamples()
{
    return ui->avgSamples->value();
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

void PreferencesDialog::setFixSecondaryEmission(bool enabled)
{
    ui->checkFixSecondary->setChecked(enabled);
}

void PreferencesDialog::setFixTriodeParameters(bool enabled)
{
    ui->checkFixTriode->setChecked(enabled);
}

bool PreferencesDialog::showScreenCurrent()
{
    return ui->checkScreenCurrent->isChecked();
}

bool PreferencesDialog::smoothCurves()
{
    return ui->checkSmoothCurves->isChecked();
}

bool PreferencesDialog::showDataTab()
{
    return ui->checkShowDataTab->isChecked();
}

bool PreferencesDialog::warmStartFromDeviceModel()
{
    return ui->checkWarmStartDeviceModel->isChecked();
}

double PreferencesDialog::grid1CommandForDesired(double desiredVoltage) const
{
    return gridCommandForDesired(desiredVoltage,
                                 grid1MeasuredLowSpinBox->value(),
                                 grid1MeasuredHighSpinBox->value());
}

double PreferencesDialog::grid2CommandForDesired(double desiredVoltage) const
{
    return gridCommandForDesired(desiredVoltage,
                                 grid2MeasuredLowSpinBox->value(),
                                 grid2MeasuredHighSpinBox->value());
}

double PreferencesDialog::getGrid1MeasuredLow() const
{
    return grid1MeasuredLowSpinBox ? grid1MeasuredLowSpinBox->value() : -GRID_CAL_LOW_REF;
}

double PreferencesDialog::getGrid1MeasuredHigh() const
{
    return grid1MeasuredHighSpinBox ? grid1MeasuredHighSpinBox->value() : -GRID_CAL_HIGH_REF;
}

double PreferencesDialog::getGrid2MeasuredLow() const
{
    return grid2MeasuredLowSpinBox ? grid2MeasuredLowSpinBox->value() : -GRID_CAL_LOW_REF;
}

double PreferencesDialog::getGrid2MeasuredHigh() const
{
    return grid2MeasuredHighSpinBox ? grid2MeasuredHighSpinBox->value() : -GRID_CAL_HIGH_REF;
}

double PreferencesDialog::gridCommandForDesired(double desiredVoltage,
                                               double measuredLow,
                                               double measuredHigh) const
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

void PreferencesDialog::setHvCalibrationRawAdc(int hv1Adc,
                                              int iaHi1Adc,
                                              int iaLo1Adc,
                                              int hv2Adc,
                                              int iaHi2Adc,
                                              int iaLo2Adc)
{
    lastHv1Adc = hv1Adc;
    lastIaHi1Adc = iaHi1Adc;
    lastIaLo1Adc = iaLo1Adc;
    lastHv2Adc = hv2Adc;
    lastIaHi2Adc = iaHi2Adc;
    lastIaLo2Adc = iaLo2Adc;

    if (hvCalRawLabel) {
        hvCalRawLabel->setText(QString("HV1=%1 IA_HI1=%2 IA_LO1=%3 | HV2=%4 IA_HI2=%5 IA_LO2=%6")
                                   .arg(hv1Adc)
                                   .arg(iaHi1Adc)
                                   .arg(iaLo1Adc)
                                   .arg(hv2Adc)
                                   .arg(iaHi2Adc)
                                   .arg(iaLo2Adc));
    }
}

void PreferencesDialog::loadFromSettings()
{
    QSettings s("ValveWorkbench", "ValveWorkbench");

    ui->checkScreenCurrent->setChecked(s.value("preferences/showScreenCurrent", true).toBool());
    ui->checkRemodel->setChecked(s.value("preferences/useRemodelling", false).toBool());
    // When enabled, pentode fitting can warm-start from a saved device model seed.
    ui->checkWarmStartDeviceModel->setChecked(s.value("preferences/warmStartFromDeviceModel", false).toBool());
    ui->checkSecondary->setChecked(s.value("preferences/useSecondaryEmission", true).toBool());
    ui->checkFixTriode->setChecked(s.value("preferences/fixTriodeParameters", true).toBool());
    ui->checkFixSecondary->setChecked(s.value("preferences/fixSecondaryEmission", true).toBool());
    ui->checkSmoothCurves->setChecked(s.value("preferences/smoothCurves", false).toBool());
    ui->checkShowDataTab->setChecked(s.value("preferences/showDataTab", false).toBool());
}

void PreferencesDialog::saveToSettings() const
{
    QSettings s("ValveWorkbench", "ValveWorkbench");

    s.setValue("preferences/showScreenCurrent", ui->checkScreenCurrent->isChecked());
    s.setValue("preferences/useRemodelling", ui->checkRemodel->isChecked());
    s.setValue("preferences/warmStartFromDeviceModel", ui->checkWarmStartDeviceModel->isChecked());
    s.setValue("preferences/useSecondaryEmission", ui->checkSecondary->isChecked());
    s.setValue("preferences/fixTriodeParameters", ui->checkFixTriode->isChecked());
    s.setValue("preferences/fixSecondaryEmission", ui->checkFixSecondary->isChecked());
    s.setValue("preferences/smoothCurves", ui->checkSmoothCurves->isChecked());
    s.setValue("preferences/showDataTab", ui->checkShowDataTab->isChecked());
    s.setValue("hvCal/linkVoltagePoints", linkHvVoltagePointsCheckBox->isChecked());

    QVariantList hv1Volts;
    QVariantList hv1HvAdc;
    QVariantList hv1IaHi;
    QVariantList hv1IaLo;
    for (int i = 0; i < hv1PointVoltsSpin.size() && i < 6; ++i) {
        hv1Volts.append(hv1PointVoltsSpin[i]->value());
        hv1HvAdc.append(hv1PointHvAdcSpin[i]->value());
        hv1IaHi.append(hv1PointIaHiAdcSpin[i]->value());
        hv1IaLo.append(hv1PointIaLoAdcSpin[i]->value());
    }
    s.setValue("hvCal/hv1/points/volts", hv1Volts);
    s.setValue("hvCal/hv1/points/hvAdc", hv1HvAdc);
    s.setValue("hvCal/hv1/points/iaHiAdc", hv1IaHi);
    s.setValue("hvCal/hv1/points/iaLoAdc", hv1IaLo);

    QVariantList hv2Volts;
    QVariantList hv2HvAdc;
    QVariantList hv2IaHi;
    QVariantList hv2IaLo;
    for (int i = 0; i < hv2PointVoltsSpin.size() && i < 6; ++i) {
        hv2Volts.append(hv2PointVoltsSpin[i]->value());
        hv2HvAdc.append(hv2PointHvAdcSpin[i]->value());
        hv2IaHi.append(hv2PointIaHiAdcSpin[i]->value());
        hv2IaLo.append(hv2PointIaLoAdcSpin[i]->value());
    }
    s.setValue("hvCal/hv2/points/volts", hv2Volts);
    s.setValue("hvCal/hv2/points/hvAdc", hv2HvAdc);
    s.setValue("hvCal/hv2/points/iaHiAdc", hv2IaHi);
    s.setValue("hvCal/hv2/points/iaLoAdc", hv2IaLo);

    auto writeCurveOrClear = [&s](const QString &xKey, const QString &yKey, const QVariantList &xs, const QVariantList &ys) {
        bool ok = (xs.size() >= 2 && ys.size() >= 2 && xs.size() == ys.size());
        if (ok) {
            double minX = xs.at(0).toDouble();
            double maxX = minX;
            for (int i = 1; i < xs.size(); ++i) {
                const double x = xs.at(i).toDouble();
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
            }
            if (!((maxX - minX) > 1e-9)) {
                ok = false;
            }
        }

        if (ok) {
            s.setValue(xKey, xs);
            s.setValue(yKey, ys);
        } else {
            s.remove(xKey);
            s.remove(yKey);
        }
    };

    auto buildVoltageCurve = [](const QVariantList &adcList, const QVariantList &vList, QVariantList &outAdc, QVariantList &outV) {
        for (int i = 0; i < adcList.size() && i < vList.size(); ++i) {
            const double v = vList.at(i).toDouble();
            const int adc = adcList.at(i).toInt();
            if (adc == 0 && std::fabs(v) < 1e-9) {
                continue;
            }
            if (adc >= 0 && v >= 0.0) {
                outAdc.append(adc);
                outV.append(v);
            }
        }
    };

    auto buildCurrentCurve = [this](const QVariantList &adcList, const QVariantList &vList, int startIdx, int count, QVariantList &outAdc, QVariantList &outmA) {
        const double rOhms = calibrationLoadResistorSpinBox ? calibrationLoadResistorSpinBox->value() : 47000.0;
        const double denom = (rOhms > 1.0) ? rOhms : 1.0;
        for (int i = 0; i < count; ++i) {
            const int idx = startIdx + i;
            if (idx >= adcList.size() || idx >= vList.size()) {
                continue;
            }
            const double v = vList.at(idx).toDouble();
            const int adc = adcList.at(idx).toInt();
            if (adc == 0 && std::fabs(v) < 1e-9) {
                continue;
            }
            if (adc >= 0 && v >= 0.0) {
                outAdc.append(adc);
                outmA.append((v / denom) * 1000.0);
            }
        }
    };

    {
        QVariantList vAdc;
        QVariantList vVolts;
        buildVoltageCurve(hv1HvAdc, hv1Volts, vAdc, vVolts);
        writeCurveOrClear("hvCal/hv1/voltage/adc", "hvCal/hv1/voltage/volts", vAdc, vVolts);

        QVariantList iHiAdc;
        QVariantList iHimA;
        buildCurrentCurve(hv1IaHi, hv1Volts, 0, 3, iHiAdc, iHimA);
        writeCurveOrClear("hvCal/hv1/currentHi/adc", "hvCal/hv1/currentHi/mA", iHiAdc, iHimA);

        QVariantList iLoAdc;
        QVariantList iLomA;
        buildCurrentCurve(hv1IaLo, hv1Volts, 3, 3, iLoAdc, iLomA);
        writeCurveOrClear("hvCal/hv1/currentLo/adc", "hvCal/hv1/currentLo/mA", iLoAdc, iLomA);
    }

    {
        QVariantList vAdc;
        QVariantList vVolts;
        buildVoltageCurve(hv2HvAdc, hv2Volts, vAdc, vVolts);
        writeCurveOrClear("hvCal/hv2/voltage/adc", "hvCal/hv2/voltage/volts", vAdc, vVolts);

        QVariantList iHiAdc;
        QVariantList iHimA;
        buildCurrentCurve(hv2IaHi, hv2Volts, 0, 3, iHiAdc, iHimA);
        writeCurveOrClear("hvCal/hv2/currentHi/adc", "hvCal/hv2/currentHi/mA", iHiAdc, iHimA);

        QVariantList iLoAdc;
        QVariantList iLomA;
        buildCurrentCurve(hv2IaLo, hv2Volts, 3, 3, iLoAdc, iLomA);
        writeCurveOrClear("hvCal/hv2/currentLo/adc", "hvCal/hv2/currentLo/mA", iLoAdc, iLomA);
    }

    s.setValue("gridCal/g1Low", grid1MeasuredLowSpinBox->value());
    s.setValue("gridCal/g1High", grid1MeasuredHighSpinBox->value());
    s.setValue("gridCal/g2Low", grid2MeasuredLowSpinBox->value());
    s.setValue("gridCal/g2High", grid2MeasuredHighSpinBox->value());
}