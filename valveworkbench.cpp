#include "valveworkbench.h"
#include "ui_valveworkbench.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QString>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QDir>
#include <QTreeWidgetItem>
#include <algorithm>

#include "analyser/analyser.h"
#include "valvemodel/model/model.h"
#include "valvemodel/model/device.h"
#include "valvemodel/model/estimate.h"
#include "valvemodel/model/modelfactory.h"
#include "valvemodel/data/project.h"
#include "valvemodel/data/measurement.h"
#include "valvemodel/data/sample.h"
#include "valvemodel/data/sweep.h"
#include "valvemodel/circuit/circuit.h"
#include "valvemodel/circuit/triodecommoncathode.h"
#include "ledindicator/ledindicator.h"
#include "preferencesdialog.h"
#include "projectdialog.h"
#include "comparedialog.h"

#include "valvemodel/circuit/sharedspice.h"

int ngspice_getchar(char* outputreturn, int ident, void* userdata) {
    // Callback for ngSpice to send characters (e.g., print output)
    // For now, just return 0
    return 0;
}

int ngspice_getstat(char* outputreturn, int ident, void* userdata) {
    // Callback for ngSpice status
    return 0;
}

int ngspice_exit(int exitstatus, int immediate, int quitexit, int ident, void* userdata) {
    // Callback for ngSpice exit
    return 0;
}

int ngspice_data(void* pvecvalues, int numvecs, int ident, void* userdata) {
    // Callback for ngSpice data
    return 0;
}

int ngspice_initdata(void* pvecinit, int ident, void* userdata) {
    // Callback for ngSpice init data
    return 0;
}

int ngspice_thread_runs(int thread_id, void* userdata) {
    // Callback for ngSpice thread
    return 0;
}

ValveWorkbench::ValveWorkbench(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ValveWorkbench)
{
    logFile = new QFile("analyser.log");
    if (!logFile->open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open log file.");
        logFile = nullptr;
    }

    // ngSpice_Init(ngspice_getchar, ngspice_getstat, ngspice_exit, ngspice_data, ngspice_initdata, ngspice_thread_runs, NULL);

    anodeStart = 0.0;
    anodeStep = 0.0;
    anodeStop = 0.0;
    gridStart = 0.0;
    gridStep = 0.0;
    gridStop = 0.0;
    screenStart = 0.0;
    screenStep = 0.0;
    screenStop = 0.0;

    secondGridStart = 0.0;
    secondGridStop = 0.0;
    secondGridStep = 0.0;

    secondAnodeStart = 0.0;
    secondAnodeStop = 0.0;
    secondAnodeStep = 0.0;

    readConfig(tr("analyser.json"));

    loadDevices();

    ui->setupUi(this);

    // Add the Data tab programmatically
    QWidget *dataTab = nullptr;
    bool dataTabExists = false;
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        if (ui->tabWidget->tabText(i) == "Data") {
            dataTab = ui->tabWidget->widget(i);
            dataTabExists = true;
            break;
        }
    }

    if (!dataTabExists) {
        dataTab = new QWidget();
        ui->tabWidget->addTab(dataTab, "Data");
    }

    // Don't manage the layout - just add widgets directly
    // The UI file should already have proper layout
    QLabel *dataLabel = new QLabel("Sweep Data Table", dataTab);

    dataTable = new QTableWidget(dataTab);
    dataTable->setRowCount(10);
    dataTable->setColumnCount(62);
    dataTable->setHorizontalHeaderLabels(QStringList() << "Va_1" << "Va_2" << "Va_3" << "Va_4" << "Va_5" << "Va_6" << "Va_7" << "Va_8" << "Va_9" << "Va_10"
                                                        << "Va_11" << "Va_12" << "Va_13" << "Va_14" << "Va_15" << "Va_16" << "Va_17" << "Va_18" << "Va_19" << "Va_20"
                                                        << "Va_21" << "Va_22" << "Va_23" << "Va_24" << "Va_25" << "Va_26" << "Va_27" << "Va_28" << "Va_29" << "Va_30"
                                                        << "Va_31" << "Va_32" << "Va_33" << "Va_34" << "Va_35" << "Va_36" << "Va_37" << "Va_38" << "Va_39" << "Va_40"
                                                        << "Va_41" << "Va_42" << "Va_43" << "Va_44" << "Va_45" << "Va_46" << "Va_47" << "Va_48" << "Va_49" << "Va_50"
                                                        << "Va_51" << "Va_52" << "Va_53" << "Va_54" << "Va_55" << "Va_56" << "Va_57" << "Va_58" << "Va_59" << "Va_60"
                                                        << "Va_61" << "Va_62");
    dataTable->setVerticalHeaderLabels(QStringList() << "Vg_1" << "Vg_2" << "Vg_3" << "Vg_4" << "Vg_5" << "Vg_6" << "Vg_7" << "Vg_8" << "Vg_9" << "Vg_10");
    dataTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    dataTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Check if dataTab already has a layout and handle it properly
    QLayout *existingLayout = dataTab->layout();
    if (existingLayout) {
        qInfo("Data tab already has layout - using existing layout");
        existingLayout->addWidget(dataLabel);
        existingLayout->addWidget(dataTable);
    } else {
        qInfo("Data tab has no layout - creating new one");
        QVBoxLayout *dataLayout = new QVBoxLayout(dataTab);
        dataLayout->addWidget(dataLabel);
        dataLayout->addWidget(dataTable);
        dataTab->setLayout(dataLayout);
    }

    // Set initial heater button state and indicator
    ui->heaterButton->setText("Heater ON");

    ui->deviceType->addItem("Triode", TRIODE);
    ui->deviceType->addItem("Pentode", PENTODE);
    ui->deviceType->addItem("Double Triode", TRIODE);
    ui->deviceType->addItem("Diode", DIODE);

    loadTemplate(0);

    //buildModelSelection();

    // ui->runButton->setEnabled(false);  // Commented out for testing

    ui->progressBar->setRange(0, 100);
    ui->progressBar->reset();
    ui->progressBar->setVisible(false);

    // Initialize heater indicator - simplified approach
    heaterIndicator = new LedIndicator();
    heaterIndicator->setOffColor(QColorConstants::LightGray);

    // Just add to layout - let Qt handle any existing widgets
    ui->heaterLayout->addWidget(heaterIndicator);
    heaterIndicator->setState(true); // Heater is always "on"

    ui->measureCheck->setVisible(false);
    ui->modelCheck->setVisible(false);
    ui->screenCheck->setVisible(false);

    ui->fitPentodeButton->setVisible(false);
    ui->fitTriodeButton->setVisible(false);

    ui->graphicsView->setScene(plot.getScene());

    connect(&serialPort, &QSerialPort::readyRead, this, &ValveWorkbench::handleReadyRead);
    connect(&serialPort, &QSerialPort::errorOccurred, this, &ValveWorkbench::handleError);
    connect(&timeoutTimer, &QTimer::timeout, this, &ValveWorkbench::handleTimeout);
    connect(ui->runButton, &QPushButton::clicked, this, &ValveWorkbench::on_runButton_clicked);

    checkComPorts();

    analyser = new Analyser(this, &serialPort, &timeoutTimer);
    analyser->setPreferences(&preferencesDialog);

    // Removed duplicate ui->graphicsView->setScene(plot.getScene());

    int count = ui->properties->rowCount();
    for (int i = 0; i < count; i++) {
        ui->properties->removeRow(0);
    }

    buildCircuitParameters();
    buildCircuitSelection();

    circuits.append(new TriodeCommonCathode());
}

ValveWorkbench::~ValveWorkbench()
{
    delete ui;
}

void ValveWorkbench::buildCircuitParameters()
{
    circuitLabels[0] = ui->cir1Label;
    circuitLabels[1] = ui->cir2Label;
    circuitLabels[2] = ui->cir3Label;
    circuitLabels[3] = ui->cir4Label;
    circuitLabels[4] = ui->cir5Label;
    circuitLabels[5] = ui->cir6Label;
    circuitLabels[6] = ui->cir7Label;
    circuitLabels[7] = ui->cir8Label;
    circuitLabels[8] = ui->cir9Label;
    circuitLabels[9] = ui->cir10Label;
    circuitLabels[10] = ui->cir11Label;
    circuitLabels[11] = ui->cir12Label;
    circuitLabels[12] = ui->cir13Label;
    circuitLabels[13] = ui->cir14Label;
    circuitLabels[14] = ui->cir15Label;
    circuitLabels[15] = ui->cir16Label;

    circuitValues[0] = ui->cir1Value;
    circuitValues[1] = ui->cir2Value;
    circuitValues[2] = ui->cir3Value;
    circuitValues[3] = ui->cir4Value;
    circuitValues[4] = ui->cir5Value;
    circuitValues[5] = ui->cir6Value;
    circuitValues[6] = ui->cir7Value;
    circuitValues[7] = ui->cir8Value;
    circuitValues[8] = ui->cir9Value;
    circuitValues[9] = ui->cir10Value;
    circuitValues[10] = ui->cir11Value;
    circuitValues[11] = ui->cir12Value;
    circuitValues[12] = ui->cir13Value;
    circuitValues[13] = ui->cir14Value;
    circuitValues[14] = ui->cir15Value;
    circuitValues[15] = ui->cir16Value;

    for (int i=0; i < 16; i++) { // Parameters all initially hidden
        circuitValues[i]->setVisible(false);
        circuitLabels[i]->setVisible(false);
    }
}

void ValveWorkbench::buildCircuitSelection()
{
    ui->circuitSelection->clear();

    ui->circuitSelection->addItem("Select...", -1);
    ui->circuitSelection->addItem("Triode Common Cathode", TRIODE_COMMON_CATHODE);
    ui->circuitSelection->addItem("Pentode Common Cathode", PENTODE_COMMON_CATHODE);
    ui->circuitSelection->addItem("AC Cathode Follower", AC_CATHODE_FOLLOWER);
    ui->circuitSelection->addItem("DC Cathode Follower", DC_CATHODE_FOLLOWER);
    ui->circuitSelection->addItem("Long Tailed Pair", LONG_TAILED_PAIR);
    ui->circuitSelection->addItem("Cathodyne Phase Splitter", CATHODYNE_PHASE_SPLITTER);
    ui->circuitSelection->addItem("Single Ended Output", SINGLE_ENDED_OUTPUT);
    ui->circuitSelection->addItem("Ultralinear Single Ended", ULTRALINEAR_SINGLE_ENDED);
    ui->circuitSelection->addItem("Push Pull Output", PUSH_PULL_OUTPUT);
    ui->circuitSelection->addItem("Ultralinear Push Pull", ULTRALINEAR_PUSH_PULL);
    ui->circuitSelection->addItem("Test Calculator", TEST_CALCULATOR);  // New test item
}

void ValveWorkbench::selectStdDevice(int index, int deviceNumber)
{
    if (deviceNumber < 0 || ui->circuitSelection->currentData().toInt() < 0) {
        return;
    }

    Device *device = devices.at(deviceNumber);
    device->anodeAxes(&plot);
    modelPlot = device->anodePlot(&plot);

    Circuit *circuit = circuits.at(ui->circuitSelection->currentData().toInt());
    if (index == 1) {
        circuit->setDevice1(device);
    } else {
        circuit->setDevice2(device);
    }
    circuit->updateUI(circuitLabels, circuitValues);
    circuit->plot(&plot);
    circuit->updateUI(circuitLabels, circuitValues);
}

void ValveWorkbench::selectModel(int modelType)
{
    customDevice->setModelType(modelType);
    //customDevice->updateUI(parameterLabels, parameterValues);
}

void ValveWorkbench::selectCircuit(int circuitType)
{
    qInfo("=== SELECTING CIRCUIT ===");
    qInfo("Circuit type: %d", circuitType);

    for (int i = 0; i < 16; i++) {
        circuitLabels[i]->setVisible(false);
        circuitValues[i]->setVisible(false);
    }

    if (circuitType < 0) {
        qInfo("Invalid circuit type - disabling device selections");
        ui->stdDeviceSelection->setCurrentIndex(0);
        ui->stdDeviceSelection2->setCurrentIndex(0);

        buildStdDeviceSelection(ui->stdDeviceSelection, -1);
        buildStdDeviceSelection(ui->stdDeviceSelection2, -1);
        return;
    }

    Circuit *circuit = circuits.at(circuitType);
    qInfo("Circuit class: %s", typeid(*circuit).name());

    circuit->setDevice1(nullptr);
    circuit->setDevice2(nullptr);

    ui->stdDeviceSelection->setCurrentIndex(0);
    ui->stdDeviceSelection2->setCurrentIndex(0);

    int deviceType1 = circuit->getDeviceType(1);
    int deviceType2 = circuit->getDeviceType(2);

    qInfo("Circuit requires device1 type: %d, device2 type: %d", deviceType1, deviceType2);

    buildStdDeviceSelection(ui->stdDeviceSelection, deviceType1);
    buildStdDeviceSelection(ui->stdDeviceSelection2, deviceType2);

    // Show parameter UI for the selected circuit
    circuit->updateUI(circuitLabels, circuitValues);

    qInfo("Circuit selection completed");
}

void ValveWorkbench::buildStdDeviceSelection(QComboBox *selection, int type)
{
    selection->clear();

    if (type < 0) {
        selection->setEnabled(false);
        return;
    }

    selection->setEnabled(true);
    selection->addItem("Select...", -1);

    qInfo("=== BUILDING DEVICE SELECTION ===");
    qInfo("Requested device type: %d", type);
    qInfo("Available devices: %d", devices.size());

    for (int i = 0; i < devices.size(); i++) {
        Device *device = devices.at(i);
        qInfo("Device %d: %s, type: %d", i, device->getName().toStdString().c_str(), device->getDeviceType());

        if (device->getDeviceType() == type) {
            selection->addItem(device->getName(), i);
            qInfo("MATCH! Added device %s to dropdown", device->getName().toStdString().c_str());
        }
    }

    qInfo("Dropdown populated with %d matching devices", selection->count() - 1); // -1 for "Select..." item
}

void ValveWorkbench::plotModel()
{
    if (modelPlot) {
       plot.getScene()->removeItem(modelPlot);
    }

    if (currentDevice != nullptr) {
        modelPlot = currentDevice->anodePlot(&plot);
    }
}

double ValveWorkbench::checkDoubleValue(QLineEdit *input, double oldValue)
{
    float parsedValue;

    const char *value = _strdup(input->text().toStdString().c_str());

    int n = sscanf_s(value, "%f.3", &parsedValue);

    if (n < 1) {
        return oldValue;
    }

    if (parsedValue < 0) {
        return 0.0;
    }

    return parsedValue;
}

void ValveWorkbench::updateDoubleValue(QLineEdit *input, double value)
{
    char number[32];

    sprintf(number, "%.3f", value);

    int length = strlen(number);
    for (int i=length-1;i >= 0; i--) {
        char test = number[i];
        if (test == '0' || test == '.') {
            number[i] = 0;
        }

        if (test != '0') {
            break;
        }
    }

    input->setText(number);
}

void ValveWorkbench::updateCircuitParameter(int index)
{
    int currentCircuitType = ui->circuitSelection->currentData().toInt();
    if (currentCircuitType < 0) {
        return; // No valid circuit selected
    }

    Circuit *circuit = circuits.at(currentCircuitType);
    double value = checkDoubleValue(circuitValues[index], circuit->getParameter(index));

    updateDoubleValue(circuitValues[index], value);
    circuit->setParameter(index, value);
    circuit->updateUI(circuitLabels, circuitValues);
    circuit->plot(&plot);
    circuit->updateUI(circuitLabels, circuitValues);
}

void ValveWorkbench::updateHeater(double vh, double ih)
{
    // Update the heater display
    if (vh >= 0.0) {
        QString vhValue = QString {"%1"}.arg(vh, -6, 'f', 3, '0');
        ui->heaterVlcd->display(vhValue);
    }

    if (ih >= 0.0) {
        QString ihValue = QString {"%1"}.arg(ih, -6, 'f', 3, '0');
        ui->heaterIlcd->display(ihValue);
    }
}

void ValveWorkbench::testProgress(int progress)
{
    // qInfo("Test progress received: %d", progress);
    //QMessageBox::information(this, "Progress", QString("Test progress: %1%").arg(progress));
    ui->progressBar->setValue(progress);
}

void ValveWorkbench::testFinished()
{
    //qInfo("Test finished");
   // QMessageBox::information(this, "Debug", "Test finished!");

    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);
    ui->btnAddToProject->setEnabled(true);

    currentMeasurement = analyser->getResult();
    measuredCurves = currentMeasurement->updatePlot(&plot);
    plot.add(measuredCurves);
    ui->measureCheck->setChecked(true);

    // Populate data table with dual rows per sweep (Va and Ia)
    if (currentMeasurement && dataTable) {
        dataTable->clearContents();
        int numSweeps = currentMeasurement->count();

        if (numSweeps == 0) {
            qWarning("No sweeps found in measurement data");
            return;
        }

        // Set up table: 6 rows per sweep for double triode (Va, Ia, Vg1, Vg3, Va2, Ia2), 4 for regular (Va, Ia, Vg1, Vg3)
        int rowsPerSweep = isDoubleTriode ? 6 : 4;
        dataTable->setRowCount(numSweeps * rowsPerSweep);

        // Set column headers for the 62 Va points
        dataTable->setColumnCount(62);
        QStringList headers;
        for (int i = 0; i < 62; ++i) {
            headers << QString("Va_%1").arg(i);
        }
        dataTable->setHorizontalHeaderLabels(headers);

        qInfo("Populating table with %d sweeps (2 rows each)", numSweeps);

        for (int sweepIdx = 0; sweepIdx < numSweeps; ++sweepIdx) {
            Sweep *sweep = currentMeasurement->at(sweepIdx);
            QString gridVoltage = QString("Vg_%1V").arg(sweep->getVg1Nominal(), 0, 'f', 2);

            int sampleCount = sweep->count();
            qInfo("Sweep %d: Vg1Nominal = %f, sampleCount = %d", sweepIdx, sweep->getVg1Nominal(), sampleCount);

            if (sampleCount == 0) {
                qWarning("Sweep %d has zero samples - skipping data population for this sweep", sweepIdx);
                continue;  // Skip to next sweep
            }

            // Row for anode voltage values
            int vaRow = sweepIdx * rowsPerSweep;
            QString vaRowHeader = gridVoltage + " (Va)";
            dataTable->setVerticalHeaderItem(vaRow, new QTableWidgetItem(vaRowHeader));

            // Row for anode current values
            int iaRow = sweepIdx * rowsPerSweep + 1;
            QString iaRowHeader = gridVoltage + " (Ia)";
            dataTable->setVerticalHeaderItem(iaRow, new QTableWidgetItem(iaRowHeader));

            // Row for first grid voltage values (Vg1)
            int vg1Row = sweepIdx * rowsPerSweep + 2;
            QString vg1RowHeader = gridVoltage + " (Vg1)";
            dataTable->setVerticalHeaderItem(vg1Row, new QTableWidgetItem(vg1RowHeader));

            // Row for second grid voltage values (Vg3)
            int vg3Row = sweepIdx * rowsPerSweep + 3;
            QString vg3RowHeader = gridVoltage + " (Vg3)";
            dataTable->setVerticalHeaderItem(vg3Row, new QTableWidgetItem(vg3RowHeader));

            int va2Row = -1;
            int ia2Row = -1;

            if (isDoubleTriode) {
                // Row for second anode voltage values (Va2)
                va2Row = sweepIdx * rowsPerSweep + 4;
                QString va2RowHeader = gridVoltage + " (Va2)";
                dataTable->setVerticalHeaderItem(va2Row, new QTableWidgetItem(va2RowHeader));

                // Row for second anode current values (Ia2)
                ia2Row = sweepIdx * rowsPerSweep + 5;
                QString ia2RowHeader = gridVoltage + " (Ia2)";
                dataTable->setVerticalHeaderItem(ia2Row, new QTableWidgetItem(ia2RowHeader));
            }

            // Populate Va row (even row numbers)
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double va = sample->getVa();
                if (col < 3) { // Log first few Va values for debugging
                    qInfo("Sweep %d, Va_%d = %f", sweepIdx, col + 1, va);
                }
                QTableWidgetItem *vaItem = new QTableWidgetItem(QString::number(va, 'f', 2));
                dataTable->setItem(vaRow, col, vaItem);
            }

            // Populate Ia row (odd row numbers)
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double ia = sample->getIa();
                if (col < 3) { // Log first few Ia values for debugging
                    qInfo("Sweep %d, Ia_%d = %f", sweepIdx, col + 1, ia);
                }
                QTableWidgetItem *iaItem = new QTableWidgetItem(QString::number(ia, 'f', 3));
                dataTable->setItem(iaRow, col, iaItem);
            }

            // Populate Vg1 row (third row per sweep)
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double vg1 = sample->getVg1();
                if (col < 3) { // Log first few Vg1 values for debugging
                    qInfo("Sweep %d, Vg1_%d = %f", sweepIdx, col + 1, vg1);
                }
                QTableWidgetItem *vg1Item = new QTableWidgetItem(QString::number(vg1, 'f', 2));
                dataTable->setItem(vg1Row, col, vg1Item);
            }

            // Populate Vg3 row (fourth row per sweep)
            for (int col = 0; col < 62 && col < sampleCount; ++col) {
                Sample *sample = sweep->at(col);
                double vg3 = sample->getVg3();
                if (col < 3) { // Log first few Vg3 values for debugging
                    qInfo("Sweep %d, Vg3_%d = %f", sweepIdx, col + 1, vg3);
                }
                QTableWidgetItem *vg3Item = new QTableWidgetItem(QString::number(vg3, 'f', 2));
                dataTable->setItem(vg3Row, col, vg3Item);
            }

            if (isDoubleTriode) {
                // Populate Va2 row (fourth row per sweep)
                for (int col = 0; col < 62 && col < sampleCount; ++col) {
                    Sample *sample = sweep->at(col);
                    double va2 = sample->getVa2();
                    if (col < 3) { // Log first few Va2 values for debugging
                        qInfo("Sweep %d, Va2_%d = %f", sweepIdx, col + 1, va2);
                    }
                    QTableWidgetItem *va2Item = new QTableWidgetItem(QString::number(va2, 'f', 2));
                    dataTable->setItem(va2Row, col, va2Item);
                }

                // Populate Ia2 row (fifth row per sweep)
                for (int col = 0; col < 62 && col < sampleCount; ++col) {
                    Sample *sample = sweep->at(col);
                    double ia2 = sample->getIa2();
                    if (col < 3) { // Log first few Ia2 values for debugging
                        qInfo("Sweep %d, Ia2_%d = %f", sweepIdx, col + 1, ia2);
                    }
                    QTableWidgetItem *ia2Item = new QTableWidgetItem(QString::number(ia2, 'f', 3));
                    dataTable->setItem(ia2Row, col, ia2Item);
                }
            }

            // Resize columns to fit content and set a minimum width for visibility
            dataTable->resizeColumnsToContents();
            for (int col = 0; col < 62; ++col) {
                dataTable->setColumnWidth(col, qMax(dataTable->columnWidth(col), 40)); // Minimum 40px width
            }
        }

        // Set row heights for better readability
        for (int row = 0; row < numSweeps * rowsPerSweep; ++row) {
            dataTable->setRowHeight(row, qMax(dataTable->rowHeight(row), 25));
        }

        qInfo("Data table populated: %d sweeps x %d rows each = %d total rows", numSweeps, rowsPerSweep, numSweeps * rowsPerSweep);
    }
}

void ValveWorkbench::testAborted()
{
    qInfo("Test aborted");
    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);
}

void ValveWorkbench::checkComPorts() {
    serialPorts = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &serialPortInfo : serialPorts) {
        if (serialPortInfo.vendorIdentifier() == 0x1a86 && serialPortInfo.productIdentifier() == 0x7523) {
            port = serialPortInfo.portName();

            setSerialPort(port);
            return;
        }
    }

    ui->tab_3->setEnabled(false);
}

void ValveWorkbench::setSerialPort(QString portName)
{
    if (serialPort.isOpen()) {
        serialPort.close();
    }

    if (portName == "") {
        ui->tab_3->setEnabled(false);
        return;
    }

    serialPort.setPortName(portName);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setBaudRate(QSerialPort::Baud115200);
    serialPort.open(QSerialPort::ReadWrite);

    ui->tab_3->setEnabled(true);
}

void ValveWorkbench::saveSamples(QString filename)
{
    QFile samplelFile(filename);

    if (!samplelFile.open(QIODevice::ReadWrite)) {
        qWarning("Couldn't open model file.");
    } else {
        QJsonObject samplesObject;

        samplesObject["name"] = ui->deviceName->text();

        samplesObject["deviceType"] = deviceType;
        samplesObject["testType"] = testType;

        samplesObject["anodeStart"] = anodeStart;
        samplesObject["anodeStop"] = anodeStop;
        samplesObject["anodeStop"] = anodeStep;

        samplesObject["gridStart"] = gridStart;
        samplesObject["gridStop"] = gridStop;
        samplesObject["gridStop"] = gridStep;

        samplesObject["screenStart"] = screenStart;
        samplesObject["screenStop"] = screenStop;
        samplesObject["screenStop"] = screenStep;

        samplesObject["vh"] = heaterVoltage;

        samplesObject["iaMax"] = iaMax;
        samplesObject["paMax"] = pMax;

        analyser->getResult()->toJson(samplesObject);

        samplelFile.write(QJsonDocument(samplesObject).toJson());
    }
}

void ValveWorkbench::readConfig(QString filename)
{
    QFile configFile(filename);

    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open config file.");
    } else {
        QByteArray configData = configFile.readAll();

        QJsonDocument configDoc(QJsonDocument::fromJson(configData));
        if (configDoc.isObject()) {
            config = configDoc.object();
        }

        if (config.contains("templates") && config["templates"].isArray()) {
            QJsonArray tpls = config["templates"].toArray();
            for (int i=0; i < tpls.count(); i++) {
                QJsonValue currentTemplate = tpls.at(i);
                if (currentTemplate.isObject()) {
                    Template *tpl = new Template();
                    tpl->read(currentTemplate.toObject());
                    templates.append(*tpl);
                }
            }
        }
    }
}

void ValveWorkbench::loadDevices()
{
    // Try multiple paths to find models - executable runs from deep build directory
    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../../../../../models",  // From release/release/bin
        QCoreApplication::applicationDirPath() + "/../../../../models",     // From release/bin
        QCoreApplication::applicationDirPath() + "/../../../models",       // From bin
        QCoreApplication::applicationDirPath() + "/../models",             // From app dir
        QCoreApplication::applicationDirPath() + "/models",                // Adjacent to app
        QDir::currentPath() + "/models",                                   // From current dir
        QDir::currentPath() + "/../models",                               // From current parent
        QDir::currentPath() + "/../../models",                            // From current grandparent
        QDir::currentPath() + "/../../../models"                          // From source root
    };

    QString modelPath;
    for (const QString& path : possiblePaths) {
        QDir testDir(path);
        if (testDir.exists()) {
            modelPath = path;
            qInfo("Found models at: %s", path.toStdString().c_str());
            break;
        } else {
            qInfo("Path not found: %s", path.toStdString().c_str());
        }
    }

    if (modelPath.isEmpty()) {
        modelPath = tr("../models"); // fallback to original
        qInfo("Using fallback path: %s", modelPath.toStdString().c_str());
    }

    QDir modelDir(modelPath);

    QStringList filters;
    filters << "*.vwm" << "*.json";  // Load both .vwm and .json files
    modelDir.setNameFilters(filters);

    QStringList models = modelDir.entryList();

    qInfo("=== LOADING DEVICES ===");
    qInfo("Application dir: %s", QCoreApplication::applicationDirPath().toStdString().c_str());
    qInfo("Current dir: %s", QDir::currentPath().toStdString().c_str());
    qInfo("Model path: %s", modelPath.toStdString().c_str());
    qInfo("Found %d model files", models.size());

    for (int i = 0; i < models.size(); i++) {
        QString modelFileName = modelPath + "/" + models.at(i);
        QFile modelFile(modelFileName);
        if (!modelFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open model file: ", modelFile.fileName().toStdString().c_str());
        }
        else {
            QByteArray modelData = modelFile.readAll();
            QJsonDocument modelDoc(QJsonDocument::fromJson(modelData));

            Device *model = new Device(modelDoc);
            qInfo("Loaded device: %s, type: %d", model->getName().toStdString().c_str(), model->getDeviceType());
            this->devices.append(model);
        }
    }

    qInfo("Total devices loaded: %d", devices.size());
}

void ValveWorkbench::loadTemplate(int index)
{
    Template tpl = templates.at(index);

    ui->deviceName->setText(tpl.getName());
    heaterVoltage = tpl.getVHeater();
    anodeStart = tpl.getVaStart();
    anodeStop = tpl.getVaStop();
    anodeStep = tpl.getVaStep();
    gridStart = tpl.getVgStart();
    gridStop = tpl.getVgStop();
    gridStep = tpl.getVgStep();
    screenStart = tpl.getVsStart();
    screenStop = tpl.getVsStop();
    screenStep = tpl.getVsStep();
    pMax = tpl.getPaMax();
    iaMax = tpl.getIaMax();

    updateParameterDisplay();

    ui->deviceType->setCurrentIndex(tpl.getDeviceType());
    on_deviceType_currentIndexChanged(tpl.getDeviceType());

    ui->testType->setCurrentIndex(tpl.getTestType());
    on_testType_currentIndexChanged(tpl.getTestType());
}


void ValveWorkbench::updateParameterDisplay()
{
    updateDoubleValue(ui->heaterVoltage, heaterVoltage);
    updateDoubleValue(ui->anodeStart, anodeStart);
    updateDoubleValue(ui->anodeStop, anodeStop);
    updateDoubleValue(ui->anodeStep, anodeStep);
    updateDoubleValue(ui->gridStart, gridStart);
    updateDoubleValue(ui->gridStop, gridStop);
    updateDoubleValue(ui->gridStep, gridStep);

    if (ui->deviceType->currentText() == "Double Triode") {
        updateDoubleValue(ui->anodeStart, anodeStart);
        updateDoubleValue(ui->anodeStop, anodeStop);
        updateDoubleValue(ui->anodeStep, anodeStep);
        updateDoubleValue(ui->gridStart, secondGridStart);
        updateDoubleValue(ui->gridStop, secondGridStop);
        updateDoubleValue(ui->gridStep, secondGridStep);
        updateDoubleValue(ui->screenStart, secondAnodeStart);
        updateDoubleValue(ui->screenStop, secondAnodeStop);
        updateDoubleValue(ui->screenStep, secondAnodeStep);
    } else {
        updateDoubleValue(ui->anodeStart, anodeStart);
        updateDoubleValue(ui->anodeStop, anodeStop);
        updateDoubleValue(ui->anodeStep, anodeStep);
        updateDoubleValue(ui->gridStart, gridStart);
        updateDoubleValue(ui->gridStop, gridStop);
        updateDoubleValue(ui->gridStep, gridStep);
        updateDoubleValue(ui->screenStart, screenStart);
        updateDoubleValue(ui->screenStop, screenStop);
        updateDoubleValue(ui->screenStep, screenStep);
    }

    updateDoubleValue(ui->pMax, pMax);
    updateDoubleValue(ui->iaMax, iaMax);
}

void ValveWorkbench::pentodeMode()
{
    updateParameterDisplay();

    deviceType = PENTODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Characteristics", ANODE_CHARACTERISTICS);
    ui->testType->addItem("Transfer Characteristics", TRANSFER_CHARACTERISTICS);
    // ui->testType->addItem("Screen Characteristics", SCREEN_CHARACTERISTICS);

    ui->gridLabel->setEnabled(true);
    ui->gridStart->setEnabled(true);
    ui->gridStop->setEnabled(true);
    ui->gridStep->setEnabled(true);

    ui->screenLabel->setEnabled(true);
    ui->screenStart->setEnabled(true);
    ui->screenStop->setEnabled(true);
    ui->screenStep->setEnabled(true);
}

void ValveWorkbench::triodeMode(bool doubleTriode)
{
    updateParameterDisplay();

    deviceType = TRIODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Characteristics", ANODE_CHARACTERISTICS);
    ui->testType->addItem("Transfer Characteristics", TRANSFER_CHARACTERISTICS);

    ui->gridLabel->setEnabled(true);
    ui->gridStart->setEnabled(true);
    ui->gridStop->setEnabled(true);
    ui->gridStep->setEnabled(true);

    if (doubleTriode) {
        ui->screenLabel->setText("Second Anode (Read-only)");
        ui->screenLabel->setEnabled(true);
        ui->screenStart->setEnabled(true);
        ui->screenStop->setEnabled(true);
        ui->screenStep->setEnabled(true);

        ui->anodeLabel->setText("First Anode");
        ui->anodeStart->setEnabled(true);
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(true);

        ui->gridLabel->setText("Second Grid");
        ui->gridStart->setEnabled(true);
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(true);

        secondGridStart = gridStart; // Use grid controls for second grid
        secondGridStop = gridStop;
        secondGridStep = gridStep;

        secondAnodeStart = anodeStart; // Auto-fill second anode with first anode values
        secondAnodeStop = anodeStop;
        secondAnodeStep = anodeStep;
    } else {
        ui->screenLabel->setEnabled(false);
        ui->screenStart->setEnabled(false);
        ui->screenStop->setEnabled(false);
        ui->screenStep->setEnabled(false);

        ui->anodeLabel->setEnabled(true);
        ui->anodeStart->setEnabled(true);
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(true);

        ui->gridLabel->setEnabled(true);
        ui->gridStart->setEnabled(true);
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(true);
    }

    updateParameterDisplay();
}

void ValveWorkbench::diodeMode()
{
    deviceType = DIODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Charcteristics", ANODE_CHARACTERISTICS);

    ui->gridLabel->setEnabled(false);
    ui->gridStart->setEnabled(false);
    ui->gridStop->setEnabled(false);
    ui->gridStep->setEnabled(false);

    ui->screenLabel->setEnabled(false);
    ui->screenStart->setEnabled(false);
    ui->screenStop->setEnabled(false);
    ui->screenStep->setEnabled(false);
}

void ValveWorkbench::log(QString message)
{
    if (logFile != nullptr) {
        logFile->write(message.toLatin1());
        logFile->write("\n");
    }
}

double ValveWorkbench::updateVoltage(QLineEdit *input, double oldValue, int electrode)
{
    double value = checkDoubleValue(input, oldValue);

    switch (electrode) {
    case HEATER:
        if (value > 16.0) {
            value = 16.0;
        }
        break;
    case GRID:
        if (value > 66.0) {
            value = 66.0;
        }
        break;
    case ANODE:
    case SCREEN:
        if (value > 540.0) {
            value = 540.0;
        }
        break;
    default:
        break;
    }

    updateDoubleValue(input, value);

    return value;
}

double ValveWorkbench::updatePMax()
{
    double value = checkDoubleValue(ui->pMax, pMax);

    if (value > 50.0) {
        value = 50.0;
    }

    updateDoubleValue(ui->pMax, value);
    pMax = value;

    return value;
}

double ValveWorkbench::updateIaMax()
{
    double value = checkDoubleValue(ui->iaMax, iaMax);

    if (value > 500.0) {
        value = 500.0;
    }

    updateDoubleValue(ui->iaMax, value);
    iaMax = value;

    return value;
}

//
// Slots
//

void ValveWorkbench::handleReadyRead()
{
    analyser->handleReadyRead();
}

void ValveWorkbench::handleError(QSerialPort::SerialPortError error)
{
    analyser->handleError(error);
}

void ValveWorkbench::handleTimeout()
{
    analyser->handleCommandTimeout();
}


void ValveWorkbench::on_stdDeviceSelection_currentIndexChanged(int index)
{
    selectStdDevice(1, ui->stdDeviceSelection->itemData(index).toInt());
}

void ValveWorkbench::on_circuitSelection_currentIndexChanged(int index)
{
    int circuitType = ui->circuitSelection->currentData().toInt();
    if (circuitType >= 0) {
        selectCircuit(circuitType);
    }
}

void ValveWorkbench::on_cir1Value_editingFinished()
{
    updateCircuitParameter(0);
}

void ValveWorkbench::on_cir2Value_editingFinished()
{
    updateCircuitParameter(1);
}

void ValveWorkbench::on_cir3Value_editingFinished()
{
    updateCircuitParameter(2);
}

void ValveWorkbench::on_cir4Value_editingFinished()
{
    updateCircuitParameter(3);
}

void ValveWorkbench::on_cir5Value_editingFinished()
{
    updateCircuitParameter(4);
}

void ValveWorkbench::on_cir6Value_editingFinished()
{
    updateCircuitParameter(5);
}

void ValveWorkbench::on_cir7Value_editingFinished()
{
    updateCircuitParameter(6);
}

void ValveWorkbench::on_cir8Value_editingFinished()
{
    updateCircuitParameter(7);
}


void ValveWorkbench::on_cir9Value_editingFinished()
{
    updateCircuitParameter(8);
}


void ValveWorkbench::on_cir10Value_editingFinished()
{
    updateCircuitParameter(9);
}


void ValveWorkbench::on_cir11Value_editingFinished()
{
    updateCircuitParameter(10);
}


void ValveWorkbench::on_cir12Value_editingFinished()
{
    updateCircuitParameter(11);
}

void ValveWorkbench::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void ValveWorkbench::on_actionPrint_triggered()
{

}

void ValveWorkbench::on_actionOptions_triggered()
{
    preferencesDialog.setPort(port);

    if (preferencesDialog.exec() == 1) {
        setSerialPort(preferencesDialog.getPort());

        pentodeModelType = preferencesDialog.getPentodeModelType();

        samplingType = preferencesDialog.getSamplingType();

        analyser->reset();
    }
}

void ValveWorkbench::on_actionLoad_Model_triggered()
{
    QString modelName = QFileDialog::getOpenFileName(this, "Open model", "", "*.json");

    if (modelName.isNull()) {
        return;
    }

    QFile modelFile(modelName);

    if (!modelFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open config file.");
    } else {
        QByteArray modelData = modelFile.readAll();
        currentDevice = new Device(QJsonDocument::fromJson(modelData));
    }
}

void ValveWorkbench::on_actionNew_Project_triggered()
{
    ProjectDialog dialog;

    if (dialog.exec() == 1) {
        Project *project = new Project();
        project->setName(dialog.getName());
        project->setDeviceType(dialog.getDeviceType());

        setSelectedTreeItem(currentProject, false);
        currentProject = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
        currentProject->setText(0, dialog.getName());
        currentProject->setIcon(0, QIcon(":/icons/valve32.png"));
        currentProject->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        currentProject->setData(0, Qt::UserRole, QVariant::fromValue((void *) project));

        project->setTreeItem(currentProject);
        setSelectedTreeItem(currentProject, true);
    }
}

void ValveWorkbench::on_actionSave_Project_triggered()
{
    if (currentProject != nullptr) {
        Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();

        QString projectName = QFileDialog::getSaveFileName(this, "Save Project", "", "*.vwp");

        if (projectName.isNull()) {
            return;
        }

        QFile projectFile(projectName);

        if (!projectFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
            qWarning("Couldn't open project file for Save.");
        } else {
            QJsonObject projectObject;

            project->toJson(projectObject);
            projectFile.write(QJsonDocument(projectObject).toJson());
        }
    }
}

void ValveWorkbench::on_actionOpen_Project_triggered()
{
    QString projectName = QFileDialog::getOpenFileName(this, "Open project", "", "*.vwp");

    if (projectName.isNull()) {
        return;
    }

    ui->tabWidget->setCurrentIndex(1);

    QFile projectFile(projectName);

    if (!projectFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Couldn't open project file for Open.");
    } else {
        QByteArray projectData = projectFile.readAll();
        Project *project = new Project();

        QJsonDocument projectDocument(QJsonDocument::fromJson(projectData));
        if (projectDocument.isObject()) {
            QJsonObject projectObject = projectDocument.object();
            if (projectObject.contains("project") && projectObject["project"].isObject()) {
                project->fromJson(projectObject["project"].toObject());
            }
        }

        setSelectedTreeItem(currentProject, false);
        currentProject = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
        currentProject->setText(0, project->getName());
        currentProject->setIcon(0, QIcon(":/icons/valve32.png"));
        currentProject->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        currentProject->setData(0, Qt::UserRole, QVariant::fromValue((void *) project));
        setSelectedTreeItem(currentProject, true);
        setFitButtons();

        project->buildTree(currentProject);
    }
}

void ValveWorkbench::on_actionClose_Project_triggered()
{
    if (currentProject != nullptr) {
        if (getProject(currentMeasurementItem) == currentProject) {
            currentMeasurementItem = nullptr;
        }
        if (getProject(currentModelItem) == currentProject) {
            currentModelItem = nullptr;
        }
        delete currentProject;
        currentProject = nullptr;
    }
}

void ValveWorkbench::on_actionExport_Model_triggered()
{
    if (currentModelItem != nullptr) {
        Model *model = (Model *) currentModelItem->data(0, Qt::UserRole).value<void *>();

        QString modelName = QFileDialog::getSaveFileName(this, "Save Project", "", "*.vwm");

        if (modelName.isNull()) {
            return;
        }

        QFile modelFile(modelName);

        if (!modelFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
            qWarning("Couldn't open model file for Save.");
        } else {
            QJsonObject modelDocument;

            QJsonObject modelObject;
            model->toJson(modelObject);

            modelDocument["model"] = modelObject;

            modelFile.write(QJsonDocument(modelDocument).toJson());
        }
    }
}

void ValveWorkbench::on_projectTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    //ui->estimateButton->setEnabled(false);
    //ui->fitButton->setEnabled(false);

    void *data = current->data(0, Qt::UserRole).value<void *>();

    bool showScreen = preferencesDialog.showScreenCurrent();

    switch(current->type()) {
    case TYP_PROJECT:
        qInfo("=== PROJECT TREE: TYP_PROJECT case triggered ===");
        setSelectedTreeItem(currentProject, false);
        currentProject = current;
        setSelectedTreeItem(currentProject, true);
        setFitButtons();
        ((Project *)data)->updateProperties(ui->properties);
        break;
    case TYP_MEASUREMENT: {
            qInfo("=== PROJECT TREE: TYP_MEASUREMENT case triggered ===");
            setSelectedTreeItem(currentMeasurementItem, false);
            currentMeasurementItem = current;
            setSelectedTreeItem(currentMeasurementItem, true);
            currentMeasurement = (Measurement *) data;

            setSelectedTreeItem(currentProject, false);
            currentProject = getProject(current);
            setSelectedTreeItem(currentProject, true);
            setFitButtons();

            currentMeasurement->updateProperties(ui->properties);
            currentMeasurement->setShowScreen(showScreen);
           // plot.add(measuredCurves);
            modelledCurves = nullptr;
            qInfo("=== BEFORE MEASUREMENT PLOT - Scene items count: %d ===", plot.getScene()->items().count());
            measuredCurves = currentMeasurement->updatePlot(&plot);
            qInfo("=== AFTER MEASUREMENT PLOT - measuredCurves items: %d, Scene items: %d ===", measuredCurves ? measuredCurves->childItems().count() : 0, plot.getScene()->items().count());
            plot.add(measuredCurves);
            qInfo("Added measuredCurves to plot");
            ui->measureCheck->setChecked(true);
            qInfo("=== PROJECT TREE: Finished TYP_MEASUREMENT case ===");
            break;
        }
    case TYP_SWEEP: {
            qInfo("=== PROJECT TREE: TYP_SWEEP case triggered ===");
            if (currentMeasurementItem != nullptr) {
                QFont font = currentMeasurementItem->font(0);
                font.setBold(false);
                currentMeasurementItem->setFont(0, font);
            }

            currentMeasurementItem = current;
            QFont font = currentMeasurementItem->font(0);
            font.setBold(true);
            currentMeasurementItem->setFont(0, font);

            QTreeWidgetItem *m = getParent(currentMeasurementItem, TYP_MEASUREMENT);

            if (m != nullptr) {
                currentMeasurement = (Measurement *) m->data(0, Qt::UserRole).value<void *>();

                setSelectedTreeItem(currentProject, false);
                currentProject = getProject(current);
                setSelectedTreeItem(currentProject, true);
                setFitButtons();

                Sweep *sweep = (Sweep *) data;
                sweep->updateProperties(ui->properties);
                qInfo("=== PROJECT TREE: About to call currentMeasurement->updatePlot(sweep) ===");
                
                // More aggressive clearing - clear plot completely before each update
                qInfo("=== BEFORE PLOT CLEAR - Scene items count: %d ===", plot.getScene()->items().count());
                plot.clear();
                qInfo("=== AFTER PLOT CLEAR - Scene items count: %d ===", plot.getScene()->items().count());
                
                // Also remove measuredCurves if it exists
                if (measuredCurves != nullptr) {
                    plot.remove(measuredCurves);
                    qInfo("Removed old measuredCurves");
                } else {
                    qInfo("measuredCurves is nullptr - no need to remove");
                }
                
                // Reset measuredCurves to nullptr before updating
                measuredCurves = nullptr;
                qInfo("Reset measuredCurves to nullptr");
                
                measuredCurves = currentMeasurement->updatePlot(&plot, sweep);
                qInfo("=== AFTER UPDATE PLOT - measuredCurves items: %d, Scene items: %d ===", measuredCurves ? measuredCurves->childItems().count() : 0, plot.getScene()->items().count());
                plot.add(measuredCurves);
                modelledCurves = nullptr;
                ui->measureCheck->setChecked(true);
                qInfo("=== PROJECT TREE: Finished TYP_SWEEP case ===");
            }
            break;
        }
    case TYP_ESTIMATE: {
            if (currentEstimateItem != nullptr) {
                QFont font = currentEstimateItem->font(0);
                font.setBold(false);
                currentEstimateItem->setFont(0, font);
            }
            currentEstimateItem = current;
            QFont font = currentEstimateItem->font(0);
            font.setBold(true);
            currentEstimateItem->setFont(0, font);
            currentProject = getProject(current);
            Estimate *estimate = (Estimate *) data;
            estimate->updateProperties(ui->properties);
            qInfo("=== BEFORE ESTIMATE PLOT - Scene items count: %d ===", plot.getScene()->items().count());
            // Clear plot before estimate plotting
            plot.clear();
            qInfo("Cleared plot before estimate plotting");
            estimatedCurves = estimate->plotModel(&plot, currentMeasurement);
            qInfo("=== AFTER ESTIMATE PLOT - estimatedCurves items: %d, Scene items: %d ===", estimatedCurves ? estimatedCurves->childItems().count() : 0, plot.getScene()->items().count());
            plot.add(estimatedCurves);
            qInfo("Added estimatedCurves to plot");
            break;
        }
    case TYP_MODEL: {
            setSelectedTreeItem(currentModelItem, false);
            currentModelItem = current;
            setSelectedTreeItem(currentModelItem, true);

            setSelectedTreeItem(currentProject, false);
            currentProject = getProject(current);
            setSelectedTreeItem(currentProject, true);
            setFitButtons();

            qInfo("=== MODEL PLOTTING: currentMeasurementItem type = %d, is null = %s ===", 
                   currentMeasurementItem ? currentMeasurementItem->type() : -1,
                   currentMeasurementItem ? "false" : "true");

            Sweep *sweep = nullptr;

            if (currentMeasurementItem != nullptr) {
                if (currentMeasurementItem->type() == TYP_SWEEP) {
                    // If currentMeasurementItem is a sweep, we're plotting a specific sweep
                    sweep = (Sweep *) currentMeasurementItem->data(0, Qt::UserRole).value<void *>();
                } else if (currentMeasurementItem->type() == TYP_MEASUREMENT) {
                    // If currentMeasurementItem is a measurement, force sweep to null for full measurement plotting
                    sweep = nullptr;
                }
                // Otherwise, leave sweep as nullptr for full measurement plotting
            }

            qInfo("=== MODEL PLOTTING: sweep is %s, about to call plotModel ===", sweep ? "NOT null" : "null");

            Model *model = (Model *) data;
            model->updateProperties(ui->properties);
            if (currentMeasurement != nullptr) {
                qInfo("=== VALVEWORKBENCH: Attempting model plotting ===");
                qInfo("Current measurement device type: %d, model type: %d",
                       currentMeasurement->getDeviceType(), model->getType());

                if ((currentMeasurement->getDeviceType() == TRIODE && model->getType() == COHEN_HELIE_TRIODE) ||
                    (currentMeasurement->getDeviceType() == PENTODE && model->getType() == GARDINER_PENTODE)) {
                    qInfo("Type check PASSED - proceeding with model plotting");
                    plot.remove(modelledCurves);
                    model->setShowScreen(showScreen);
                    modelledCurves = model->plotModel(&plot, currentMeasurement, sweep);
                    plot.add(modelledCurves);
                    qInfo("Model plotting completed");
                } else {
                    qInfo("Type check FAILED - skipping model plotting");
                    qInfo("Measurement device: %d, Model type: %d", currentMeasurement->getDeviceType(), model->getType());
                }
            } else {
                qInfo("No current measurement available for model plotting");
            }
            ui->modelCheck->setChecked(true);
            break;
        }       
    case TYP_SAMPLE: {
            if (currentMeasurementItem != nullptr) {
                QFont font = currentMeasurementItem->font(0);
                font.setBold(false);
                currentMeasurementItem->setFont(0, font);
            }
            currentMeasurementItem = current;
            QFont font = currentMeasurementItem->font(0);
            font.setBold(true);
            currentMeasurementItem->setFont(0, font);
            currentMeasurement = (Measurement *) data;

            setSelectedTreeItem(currentProject, false);
            currentProject = getProject(current);
            setSelectedTreeItem(currentProject, true);
            setFitButtons();

            Sample *sample = (Sample *) data;
            sample->updateProperties(ui->properties);
            break;
        }
    default:
        break;
    }
}

QTreeWidgetItem *ValveWorkbench::getProject(QTreeWidgetItem *current)
{
    if (current == nullptr) {
        return nullptr;
    }

    QTreeWidgetItem *parent = current->parent();
    if (parent != nullptr) {
        if (parent->type() == TYP_PROJECT) {
            //return (Project *) parent->data(0, Qt::UserRole).value<void *>();
            return parent;
        } else {
            return getProject(parent);
        }
    }

    return nullptr;
}

QTreeWidgetItem *ValveWorkbench::getParent(QTreeWidgetItem *current, int type)
{
    if (current == nullptr) {
        return nullptr;
    }

    QTreeWidgetItem *parent = current->parent();
    if (parent != nullptr) {
        if (parent->type() == type) {
            return parent;
        } else {
            return getProject(parent);
        }
    }

    return nullptr;
}

Model *ValveWorkbench::findModel(int type)
{
    int children = currentProject->childCount();
    Model *foundModel = nullptr;

    for (int i = 0; i < children && foundModel == nullptr; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MODEL) {
            Model *model = (Model *) child->data(0, Qt::UserRole).value<void *>();
            if (model->getType() == type) {
                foundModel = model;
            }
        }
    }

    return foundModel;
}

Measurement *ValveWorkbench::findMeasurement(int deviceType, int testType)
{
    int children = currentProject->childCount();
    Measurement *foundMeasurement = nullptr;

    for (int i = 0; i < children && foundMeasurement == nullptr; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MEASUREMENT) {
            Measurement *measurement = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (measurement->getDeviceType() == deviceType && measurement->getTestType() == testType) {
                foundMeasurement = measurement;
            }
        }
    }

    return foundMeasurement;
}

void ValveWorkbench::setSelectedTreeItem(QTreeWidgetItem *item, bool selected)
{
    if (item != nullptr) {
        QFont font = item->font(0);
        font.setBold(selected);
        item->setFont(0, font);
    }
}

void ValveWorkbench::setFitButtons()
{
    if (currentProject == nullptr) {
        ui->fitTriodeButton->setVisible(false);
        ui->fitPentodeButton->setVisible(false);
    } else {
        Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
        if (project->getDeviceType() == TRIODE) {
            ui->fitTriodeButton->setVisible(true);
            ui->fitPentodeButton->setVisible(false);
        } else {
            ui->fitTriodeButton->setVisible(false);
            ui->fitPentodeButton->setVisible(true);
        }
    }
}

void ValveWorkbench::on_deviceType_currentIndexChanged(int index)
{
    switch (ui->deviceType->itemData(index).toInt()) {
    case PENTODE:
        pentodeMode();
        break;
    case TRIODE:
        triodeMode(ui->deviceType->currentText() == "Double Triode");
        isDoubleTriode = ui->deviceType->currentText() == "Double Triode";
        break;
    case DIODE:
        diodeMode();
        break;
    default:
        break;
    }

    ui->testType->setCurrentIndex(0);
    on_testType_currentIndexChanged(0);
}

void ValveWorkbench::on_testType_currentIndexChanged(int index)
{
    updateParameterDisplay();

    if (deviceType == TRIODE) {
        ui->screenStart->setText("");
        ui->screenStop->setText("");
        ui->screenStep->setText("");
    }

    switch (ui->testType->itemData(index).toInt()) {
    case ANODE_CHARACTERISTICS: // Anode swept and Grid stepped
        ui->anodeStop->setEnabled(true);
        ui->anodeStep->setEnabled(false);
        ui->anodeStep->setText("");
        if (deviceType != DIODE) {
            ui->gridStop->setEnabled(true);
            ui->gridStep->setEnabled(true);
        }
        if (deviceType == PENTODE) { // Screen fixed (if Pentode)
            ui->screenStop->setEnabled(false);
            ui->screenStop->setText("");
            ui->screenStep->setEnabled(false);
            ui->screenStep->setText("");
        }
        break;
    case TRANSFER_CHARACTERISTICS: // Grid swept
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(false);
        ui->gridStep->setText("");
        if (deviceType == PENTODE) { // Anode fixed and Screen stepped
            ui->anodeStop->setEnabled(false);
            ui->anodeStop->setText("");
            ui->anodeStep->setEnabled(false);
            ui->anodeStep->setText("");
            ui->screenStop->setEnabled(true);
            ui->screenStep->setEnabled(true);
        } else { // (Triode) Anode stepped and no Screen
            ui->anodeStop->setEnabled(true);
            ui->anodeStep->setEnabled(true);
        }
        break;
    case SCREEN_CHARACTERISTICS: // Anode fixed, Screen swept and Grid stepped
        ui->anodeStop->setEnabled(false);
        ui->anodeStep->setEnabled(false);
        ui->gridStop->setEnabled(true);
        ui->gridStep->setEnabled(true);
        ui->screenStop->setEnabled(true);
        ui->screenStep->setEnabled(false);
        break;
    default:
        break;
    }

    testType = ui->testType->itemData(index).toInt();
}

void ValveWorkbench::on_anodeStart_editingFinished()
{
    double value = updateVoltage(ui->anodeStart, anodeStart, ANODE);
    anodeStart = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStart = value;
        updateDoubleValue(ui->screenStart, secondAnodeStart);
    }
}

void ValveWorkbench::on_anodeStop_editingFinished()
{
    double value = updateVoltage(ui->anodeStop, anodeStop, ANODE);
    anodeStop = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStop = value;
        updateDoubleValue(ui->screenStop, secondAnodeStop);
    }
}

void ValveWorkbench::on_anodeStep_editingFinished()
{
    double value = updateVoltage(ui->anodeStep, anodeStep, ANODE);
    anodeStep = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStep = value;
        updateDoubleValue(ui->screenStep, secondAnodeStep);
    }
}

void ValveWorkbench::on_gridStart_editingFinished()
{
    double value = updateVoltage(ui->gridStart, gridStart, GRID);
    gridStart = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondGridStart = value;
    }
}

void ValveWorkbench::on_gridStop_editingFinished()
{
    double value = updateVoltage(ui->gridStop, gridStop, GRID);
    gridStop = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondGridStop = value;
    }
}

void ValveWorkbench::on_gridStep_editingFinished()
{
    double value = updateVoltage(ui->gridStep, gridStep, GRID);
    gridStep = value;
    if (ui->deviceType->currentText() == "Double Triode") {
        secondGridStep = value;
    }
}

void ValveWorkbench::on_screenStart_editingFinished()
{
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStart = updateVoltage(ui->screenStart, secondAnodeStart, ANODE);
    } else {
        screenStart = updateVoltage(ui->screenStart, screenStart, SCREEN);
    }
}

void ValveWorkbench::on_screenStop_editingFinished()
{
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStop = updateVoltage(ui->screenStop, secondAnodeStop, ANODE);
    } else {
        screenStop = updateVoltage(ui->screenStop, screenStop, SCREEN);
    }
}

void ValveWorkbench::on_screenStep_editingFinished()
{
    if (ui->deviceType->currentText() == "Double Triode") {
        secondAnodeStep = updateVoltage(ui->screenStep, secondAnodeStep, ANODE);
    } else {
        screenStep = updateVoltage(ui->screenStep, screenStep, SCREEN);
    }
}

void ValveWorkbench::on_heaterVoltage_editingFinished()
{
    heaterVoltage = updateVoltage(ui->heaterVoltage, heaterVoltage, HEATER);
}

void ValveWorkbench::on_iaMax_editingFinished()
{
    updateIaMax();
}


void ValveWorkbench::on_pMax_editingFinished()
{
    updatePMax();
}
void ValveWorkbench::on_runButton_clicked()
{
    static int clickCount = 0;
    clickCount++;
    qInfo("on_runButton_clicked called (count: %d)", clickCount);

    log("Run Test button clicked");

    if (analyser == nullptr) {
        log("Error: Analyser is null");
        QMessageBox::warning(this, "Error", "Analyser not initialized");
        return;
    }

    ui->runButton->setChecked(true);
    ui->progressBar->reset();
    ui->progressBar->setVisible(true);
    ui->btnAddToProject->setEnabled(false);

    log("Configuring analyser");
    analyser->setDeviceType(deviceType);
    analyser->setTestType(testType);
    analyser->setIsDoubleTriode(ui->deviceType->currentText() == "Double Triode");
    isDoubleTriode = ui->deviceType->currentText() == "Double Triode";
    analyser->setPMax(pMax);
    analyser->setIaMax(iaMax);
    analyser->setSweepParameters(anodeStart, anodeStop, anodeStep, gridStart, gridStop, gridStep, screenStart, screenStop, screenStep, secondGridStart, secondGridStop, secondGridStep, secondAnodeStart, secondAnodeStop, secondAnodeStep);

    qInfo("Analyser parameters: anodeStart=%f, anodeStop=%f, anodeStep=%f, gridStart=%f, gridStop=%f, gridStep=%f, screenStart=%f, screenStop=%f, screenStep=%f", anodeStart, anodeStop, anodeStep, gridStart, gridStop, gridStep, screenStart, screenStop, screenStep);

    log("Starting test");
    analyser->startTest();
}

void ValveWorkbench::on_btnAddToProject_clicked()
{
    qDebug("Save to Project button clicked");
    if (currentProject == nullptr) {
        qDebug("No current project, creating new one");
        on_actionNew_Project_triggered();
    }

    Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
    qDebug("Project pointer: %p", project);
    Measurement *measurement = analyser->getResult();
    qDebug("Measurement pointer: %p", measurement);
    if (measurement == nullptr) {
        qWarning("Measurement is null - cannot add to project");
        return;
    }
    if (project->addMeasurement(measurement)) {
        qDebug("Measurement added to project successfully");
        qDebug("About to build tree");
        measurement->buildTree(currentProject);
        qDebug("Tree built successfully");
        qDebug("About to switch tab");
        // Temporarily commented out due to Qt bug in tab switching
        // ui->tabWidget->setCurrentIndex(1);
        qDebug("Tab switch commented out for workaround");
        qDebug("Setting button enabled to false");
        ui->btnAddToProject->setEnabled(false);
        qDebug("Save to Project function completed");
    } else {
        qWarning("Failed to add measurement to project");
    }

    ui->btnAddToProject->setEnabled(false);
}
void ValveWorkbench::on_fitTriodeButton_clicked()
{
    modelProject = currentProject;
    ui->fitPentodeButton->setEnabled(false); // Prevent any further modelling invocations
    ui->fitTriodeButton->setEnabled(false);
    doPentodeModel = false;

    modelTriode();
}

void ValveWorkbench::modelTriode()
{
    QList<Measurement *> measurements;

    Measurement *measurement = findMeasurement(TRIODE, ANODE_CHARACTERISTICS);

    if (measurement == nullptr) {
        QMessageBox message;
        message.setText("There is no Triode Anode Characteristic measurement in the project - this is required for model fitting");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    Estimate estimate;
    estimate.estimateTriode(measurement);

    model = ModelFactory::createModel(COHEN_HELIE_TRIODE);
    model->setEstimate(&estimate);

    int children = currentProject->childCount();
    for (int i = 0; i < children; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MEASUREMENT) {
            Measurement *measurement = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (measurement->getDeviceType() == TRIODE) {
                model->addMeasurement(measurement);
            }
        }
    }

    thread = new QThread;

    model->moveToThread(thread);
    connect(thread, &QThread::started, model, &Model::solveThreaded);
    connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void ValveWorkbench::loadModel()
{
    thread->quit();

    if (!model->isConverged()) {
        QMessageBox message;
        message.setText("The model fitting did not converge - please check that your measurements are valid");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    Project *project = (Project *) modelProject->data(0, Qt::UserRole).value<void *>();
    project->addModel(model);
    model->buildTree(modelProject);

    if (doPentodeModel) {
        modelPentode(); // Will be done in a new thread
    } else {
        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        modelProject = nullptr;
    }
}

void ValveWorkbench::on_fitPentodeButton_clicked()
{
    modelProject = currentProject;
    ui->fitPentodeButton->setEnabled(false); // Prevent any further modelling invocations
    ui->fitTriodeButton->setEnabled(false);
    doPentodeModel = true;

    CohenHelieTriode *triodeModel = (CohenHelieTriode *) findModel(COHEN_HELIE_TRIODE);

    if (triodeModel == nullptr) {
        modelTriode();
    } else {
        modelPentode();
    }

}

void ValveWorkbench::modelPentode()
{
    doPentodeModel = false; // We're doing it now so don't want to do it again!

    CohenHelieTriode *triodeModel = (CohenHelieTriode *) findModel(COHEN_HELIE_TRIODE);

    if (triodeModel == nullptr) { // Any error message will have already been displayed
        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    Measurement *measurement = findMeasurement(PENTODE, ANODE_CHARACTERISTICS);

    if (measurement == nullptr) {
        QMessageBox message;
        message.setText("There is no Pentode Anode Characteristic measurement in the project - this is required for model fitting");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        return;
    }

    Estimate estimate;
    estimate.estimatePentode(measurement, triodeModel, pentodeModelType, false);

    model = ModelFactory::createModel(pentodeModelType);
    model->setEstimate(&estimate);
    model->setMode(NORMAL_MODE);
    model->setPreferences(&preferencesDialog);

    int children = currentProject->childCount();
    for (int i = 0; i < children; i++) {
        QTreeWidgetItem *child = currentProject->child(i);
        if (child->type() == TYP_MEASUREMENT) {
            measurement = (Measurement *) child->data(0, Qt::UserRole).value<void *>();
            if (measurement->getDeviceType() == PENTODE) {
                model->addMeasurement(measurement);
            }
        }
    }

    thread = new QThread;

    model->moveToThread(thread);
    connect(model, &Model::modelReady, this, &ValveWorkbench::modelScreen);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();

    QMetaObject::invokeMethod(model, "solveThreaded");
}

void ValveWorkbench::modelScreen()
{
    if (!model->isConverged()) {
        QMessageBox message;
        message.setText("The anode current fitting did not converge - please check that your measurements are valid");
        message.exec();

        ui->fitPentodeButton->setEnabled(true); // Allow modelling again
        ui->fitTriodeButton->setEnabled(true);

        thread->quit();

        return;
    }

    model->setMode(SCREEN_MODE);

    disconnect(model, &Model::modelReady, this, &ValveWorkbench::modelScreen); // We don't want to go round the loop again!

    if (preferencesDialog.useRemodelling()) {
        connect(model, &Model::modelReady, this, &ValveWorkbench::remodelAnode);
    } else {
        connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);
    }

    QMetaObject::invokeMethod(model, "solveThreaded");
}

void ValveWorkbench::remodelAnode()
{
    model->setMode(ANODE_REMODEL_MODE);

    disconnect(model, &Model::modelReady, this, &ValveWorkbench::remodelAnode);
    connect(model, &Model::modelReady, this, &ValveWorkbench::loadModel);

    QMetaObject::invokeMethod(model, "solveThreaded");
}

void ValveWorkbench::on_tabWidget_currentChanged(int index)
{
    switch (index) {
    case 0:
        ui->measureCheck->setVisible(false);
        ui->modelCheck->setVisible(false);
        break;
    case 1:
        ui->measureCheck->setVisible(true);
        ui->modelCheck->setVisible(true);
        if (currentProject != nullptr) {
            Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
            if (project->getDeviceType() == TRIODE) {
                ui->fitTriodeButton->setVisible(true);
                ui->fitPentodeButton->setVisible(false);
            } else if (project->getDeviceType() == PENTODE) {
                ui->fitTriodeButton->setVisible(false);
                ui->fitPentodeButton->setVisible(true);
            }
        } else {
            ui->fitTriodeButton->setVisible(false);
            ui->fitPentodeButton->setVisible(false);
        }
        break;
    case 2:
        ui->measureCheck->setVisible(false);
        ui->modelCheck->setVisible(false);
        break;
    default:
        break;
    }
}


void ValveWorkbench::on_measureCheck_stateChanged(int arg1)
{
    if (measuredCurves != nullptr) {
        measuredCurves->setVisible(ui->measureCheck->isChecked());
    }
}


void ValveWorkbench::on_modelCheck_stateChanged(int arg1)
{
    if (modelledCurves != nullptr) {
        modelledCurves->setVisible(ui->modelCheck->isChecked());
    }
}


void ValveWorkbench::on_properties_itemChanged(QTableWidgetItem *item)
{
    DataSet *dataSet = item->data(Qt::UserRole).value<DataSet *>();
    if (dataSet != nullptr) {
        dataSet->editCallback(item);
    }
}


void ValveWorkbench::on_compareButton_clicked()
{
    if (currentProject == nullptr) {
        QMessageBox message;
        message.setText("No project selected");
        message.exec();

        return;
    }

    CompareDialog dialog;

    Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
    Model *model;
    if (project->getDeviceType() == TRIODE) {
        model = findModel(COHEN_HELIE_TRIODE);
    } else {
        model = findModel(GARDINER_PENTODE);
    }

    if (model == nullptr) {
        QMessageBox message;
        message.setText("No model found");
        message.exec();

        return;
    }

    dialog.setModel(model);

    dialog.exec();
}

void ValveWorkbench::on_heaterButton_clicked()
{
    // Always keep heaters as true for workflow to proceed
   // heaters = true;

    if (analyser != nullptr) {
        analyser->setIsHeatersOn(true);
    }

    if (heaterIndicator != nullptr) {
        heaterIndicator->setState(true);
    }

    // Button shows "Heater ON" to indicate heaters are on and ready
    ui->heaterButton->setText("Heater ON");
    log("Heater confirmed ON - ready for testing");
}
