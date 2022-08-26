#include "valveworkbench.h"
#include "ui_valveworkbench.h"

#include "valvemodel/circuit/triodecommoncathode.h"

ValveWorkbench::ValveWorkbench(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ValveWorkbench)
{
    logFile = new QFile("analyser.log");
    if (!logFile->open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open log file.");
        logFile = nullptr;
    }

    readConfig(tr("analyser.json"));

    ui->setupUi(this);

    ui->deviceType->addItem("Triode", TRIODE);
    ui->deviceType->addItem("Pentode", PENTODE);
    //ui->deviceType->addItem("Double Triode", TRIODE);
    //ui->deviceType->addItem("Diode", DIODE);

    loadTemplate(0);

    buildModelSelection();

    ui->runButton->setEnabled(false);

    ui->progressBar->setRange(0, 100);
    ui->progressBar->reset();
    ui->progressBar->setVisible(false);

    heaterIndicator = new LedIndicator();
    heaterIndicator->setOffColor(QColorConstants::LightGray);
    ui->heaterLayout->addWidget(heaterIndicator);

    ui->graphicsView->setScene(plot.getScene());

    connect(&serialPort, &QSerialPort::readyRead, this, &ValveWorkbench::handleReadyRead);
    connect(&serialPort, &QSerialPort::errorOccurred, this, &ValveWorkbench::handleError);
    connect(&timeoutTimer, &QTimer::timeout, this, &ValveWorkbench::handleTimeout);
    connect(&heaterTimer, &QTimer::timeout, this, &ValveWorkbench::handleHeaterTimeout);

    checkComPorts();

    analyser = new Analyser(this, &serialPort, &timeoutTimer, &heaterTimer);



    ui->setupUi(this);

    ui->graphicsView->setScene(plot.getScene());

    int count = ui->properties->rowCount();
    for (int i = 0; i < count; i++) {
        ui->properties->removeRow(0);
    }

    buildCircuitParameters();
    buildStdDeviceSelection();

    circuits.append(new TriodeCommonCathode());
}

ValveWorkbench::~ValveWorkbench()
{
    delete ui;
}

void ValveWorkbench::buildStdDeviceSelection()
{
    ui->stdDeviceSelection->addItem("Select...", -1);
    QString modelPath = tr("../models");
    QDir modelDir(modelPath);

    QStringList filters;
    filters << "*.json";
    modelDir.setNameFilters(filters);

    QStringList models = modelDir.entryList();

    for (int i = 0; i < models.size(); i++) {
        QString modelFileName = modelPath + "/" + models.at(i);
        QFile modelFile(modelFileName);
        if (!modelFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open model file: ", modelFile.fileName().toStdString().c_str());
        }
        else {
            QByteArray modelData = modelFile.readAll();
            QJsonDocument modelDoc(QJsonDocument::fromJson(modelData));

            Device* model = new Device(modelDoc);
            this->devices.append(model);
            ui->stdDeviceSelection->addItem(model->getName(), i);
        }
    }
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

    circuitValues[0] = ui->cir1Value;
    circuitValues[1] = ui->cir2Value;
    circuitValues[2] = ui->cir3Value;
    circuitValues[3] = ui->cir4Value;
    circuitValues[4] = ui->cir5Value;
    circuitValues[5] = ui->cir6Value;
    circuitValues[6] = ui->cir7Value;

    for (int i=0; i < 7; i++) { // Parameters all initially hidden
        circuitValues[i]->setVisible(false);
        circuitLabels[i]->setVisible(false);
    }
}

void ValveWorkbench::buildCircuitSelection()
{
    ui->circuitSelection->clear();

    if (currentDevice != nullptr) {
        if (currentDevice->getDeviceType() == MODEL_TRIODE) {
            ui->circuitSelection->addItem("Common Cathode", TRIODE_COMMON_CATHODE);
        } else if (currentDevice->getDeviceType() == MODEL_PENTODE) {
            ui->circuitSelection->addItem("Common Cathode", PENTODE_COMMON_CATHODE);
        }
    }
}

void ValveWorkbench::selectStdDevice(int device)
{
    if (device < 0) {
        currentDevice = customDevice;
        //setCustomModelEnabled(true);
    }
    else {
        if (device < devices.size()) {
            currentDevice = devices.at(device);
            //setCustomModelEnabled(false);
            currentDevice->anodeAxes(&plot);
            modelPlot = nullptr;
            currentDevice->updateModelSelect(ui->stdModelSelection);
            buildCircuitSelection();
            selectCircuit(ui->circuitSelection->currentData().toInt());
        }
    }
}

void ValveWorkbench::selectStdModel(int model)
{
    currentDevice->selectModel(model);
    //currentDevice->updateUI(parameterLabels, parameterValues);
    plotModel();
}

void ValveWorkbench::selectModel(int modelType)
{
    customDevice->setModelType(modelType);
    //customDevice->updateUI(parameterLabels, parameterValues);
}

void ValveWorkbench::selectCircuit(int circuitType)
{
    Circuit *circuit = circuits.at(circuitType);
    if (circuitType < circuits.size()) {
        circuit->updateUI(circuitLabels, circuitValues);
        circuit->plot(&plot, currentDevice);
        circuit->updateUI(circuitLabels, circuitValues);
    }
}

void ValveWorkbench::selectPlot(int plotType)
{
    plotModel();
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
    Circuit *circuit = circuits.at(ui->circuitSelection->currentData().toInt());
    double value = checkDoubleValue(circuitValues[index], circuit->getParameter(index));

    updateDoubleValue(circuitValues[index], value);
    circuit->setParameter(index, value);
    circuit->updateUI(circuitLabels, circuitValues);
    circuit->plot(&plot, currentDevice);
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
    ui->progressBar->setValue(progress);
}

void ValveWorkbench::testFinished()
{
    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);

    buildModelSelection();

    doPlot();
}

void ValveWorkbench::testAborted()
{
    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);
}

void ValveWorkbench::checkComPorts() {
    serialPorts = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &serialPortInfo : serialPorts) {
        if (serialPortInfo.vendorIdentifier() == 0x1a86 && serialPortInfo.productIdentifier() == 0x7523) {
            port = serialPortInfo.portName();
        }
    }

    setSerialPort(port);
}

void ValveWorkbench::setSerialPort(QString portName)
{
    if (serialPort.isOpen()) {
        serialPort.close();
    }

    serialPort.setPortName(portName);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setBaudRate(QSerialPort::Baud115200);
    serialPort.open(QSerialPort::ReadWrite);
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

void ValveWorkbench::pentodeMode()
{
    deviceType = PENTODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Characteristics", ANODE_CHARACTERISTICS);
    ui->testType->addItem("Transfer Characteristics", TRANSFER_CHARACTERISTICS);
    ui->testType->addItem("Screen Characteristics", SCREEN_CHARACTERISTICS);

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
    deviceType = TRIODE;

    ui->testType->clear();
    ui->testType->addItem("Anode Characteristics", ANODE_CHARACTERISTICS);
    ui->testType->addItem("Transfer Characteristics", TRANSFER_CHARACTERISTICS);

    ui->gridLabel->setEnabled(true);
    ui->gridStart->setEnabled(true);
    ui->gridStop->setEnabled(true);
    ui->gridStep->setEnabled(true);

    ui->screenLabel->setEnabled(false);
    ui->screenStart->setEnabled(false);
    ui->screenStop->setEnabled(false);
    ui->screenStep->setEnabled(false);
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

    if (value > 200.0) {
        value = 200.0;
    }

    updateDoubleValue(ui->iaMax, value);
    iaMax = value;

    return value;
}

//
// Slots
//

void ValveAnalyser::handleReadyRead()
{
    analyser->handleReadyRead();
}

void ValveAnalyser::handleError(QSerialPort::SerialPortError error)
{
    analyser->handleError(error);
}

void ValveAnalyser::handleTimeout()
{
    analyser->handleCommandTimeout();
}

void ValveAnalyser::handleHeaterTimeout()
{
    analyser->handleHeaterTimeout();
}

void ValveWorkbench::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void ValveWorkbench::on_actionPrint_triggered()
{

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

void ValveWorkbench::on_stdDeviceSelection_currentIndexChanged(int index)
{
    selectStdDevice(ui->stdDeviceSelection->currentData().toInt());
}

void ValveWorkbench::on_stdModelSelection_currentIndexChanged(int index)
{
    selectStdModel(ui->stdModelSelection->currentIndex());
}

void ValveWorkbench::on_circuitSelection_currentIndexChanged(int index)
{
    selectCircuit(ui->circuitSelection->currentData().toInt());
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

void ValveWorkbench::on_actionNew_Project_triggered()
{
    QTreeWidgetItem *project = new QTreeWidgetItem(ui->projectTree, TYP_PROJECT);
    project->setText(0, "New project");
    project->setIcon(0, QIcon(":/icons/valve32.png"));
    project->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void ValveWorkbench::on_actionLoad_Measurement_triggered()
{
    QTreeWidgetItem *currentItem = ui->projectTree->currentItem();
    if (currentItem->type() == TYP_PROJECT) {
        QString measurementlName = QFileDialog::getOpenFileName(this, "Open measurement", "", "*.json");

        if (measurementlName.isNull()) {
            return;
        }

        QFile measurementFile(measurementlName);

        if (!measurementFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open measurement file.");
        } else {
            QByteArray measurementData = measurementFile.readAll();
            Measurement *measurement = new Measurement();
            measurement->fromJson(QJsonDocument::fromJson(measurementData).object());
            measurement->buildTree(currentItem);
        }
    }
}


void ValveWorkbench::on_projectTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    switch(current->type()) {
    case TYP_PROJECT:
        break;
    default:
        DataSet *dataSet = (DataSet *) current->data(0, Qt::UserRole).value<void *>();
        dataSet->updateProperties(ui->properties);
        dataSet->updatePlot(&plot);
        break;
    }
}
