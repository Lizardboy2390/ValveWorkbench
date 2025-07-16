#include "valveworkbench.h"
#include "ui_valveworkbench.h"

#include "valvemodel/circuit/triodecommoncathode.h"
#include "valvemodel/circuit/pentodecommoncathode.h"
#include "valvemodel/data/sweep.h"

#include "preferencesdialog.h"
#include "projectdialog.h"
// #include "comparedialog.h" // Temporarily disabled

#include <QMessageBox>
#include <QCoreApplication>

ValveWorkbench::ValveWorkbench(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ValveWorkbench)
{
    logFile = new QFile("analyser.log");
    if (!logFile->open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open log file.");
        logFile = nullptr;
    }

    ngSpice_Init(ng_getchar, ng_getstat, ng_exit, ng_data, ng_initdata, ng_thread_runs, NULL);

    anodeStart = 0.0;
    anodeStep = 0.0;
    anodeStop = 0.0;
    gridStart = 0.0;
    gridStep = 0.0;
    gridStop = 0.0;
    screenStart = 0.0;
    screenStep = 0.0;
    screenStop = 0.0;

    // Use absolute path to ensure the config file is found
    readConfig(QCoreApplication::applicationDirPath() + "/analyser.json");

    loadDevices();

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

    ui->measureCheck->setVisible(false);
    ui->modelCheck->setVisible(false);
    ui->screenCheck->setVisible(false);

    ui->fitPentodeButton->setVisible(false);
    ui->fitTriodeButton->setVisible(false);

    ui->graphicsView->setScene(plot.getScene());

    connect(&serialPort, &QSerialPort::readyRead, this, &ValveWorkbench::handleReadyRead);
    connect(&serialPort, &QSerialPort::errorOccurred, this, &ValveWorkbench::handleError);
    connect(&timeoutTimer, &QTimer::timeout, this, &ValveWorkbench::handleTimeout);
    connect(&heaterTimer, &QTimer::timeout, this, &ValveWorkbench::handleHeaterTimeout);

    checkComPorts();

    analyser = new Analyser(this, &serialPort, &timeoutTimer, &heaterTimer);
    analyser->setPreferences(&preferencesDialog);

    ui->graphicsView->setScene(plot.getScene());

    int count = ui->properties->rowCount();
    for (int i = 0; i < count; i++) {
        ui->properties->removeRow(0);
    }

    buildCircuitParameters();
    buildCircuitSelection();

    circuits.append(new TriodeCommonCathode());
    circuits.append(new PentodeCommonCathode());
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

    /*if (currentDevice != nullptr) {
        if (currentDevice->getDeviceType() == MODEL_TRIODE) {
            ui->circuitSelection->addItem("Common Cathode", TRIODE_COMMON_CATHODE);
        } else if (currentDevice->getDeviceType() == MODEL_PENTODE) {
            ui->circuitSelection->addItem("Common Cathode", PENTODE_COMMON_CATHODE);
        }
    }*/
}

void ValveWorkbench::selectStdDevice(int index, int deviceNumber)
{
    // Check if device number and circuit selection are valid
    int circuitIndex = ui->circuitSelection->currentData().toInt();
    if (deviceNumber < 0 || circuitIndex < 0) {
        return;
    }
    
    // Check if device index is valid
    if (deviceNumber >= devices.size()) {
        qWarning("Invalid device number: %d", deviceNumber);
        return;
    }
    
    // Check if circuit index is valid
    if (circuitIndex >= circuits.size()) {
        qWarning("Invalid circuit index: %d", circuitIndex);
        return;
    }

    Device *device = devices.at(deviceNumber);
    device->anodeAxes(&plot);
    modelPlot = device->anodePlot(&plot);

    Circuit *circuit = circuits.at(circuitIndex);
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
    for (int i = 0; i < 16; i++) {
        circuitLabels[i]->setVisible(false);
        circuitValues[i]->setVisible(false);
    }

    if (circuitType < 0) {
        ui->stdDeviceSelection->setCurrentIndex(0);
        ui->stdDeviceSelection2->setCurrentIndex(0);

        buildStdDeviceSelection(ui->stdDeviceSelection, -1);
        buildStdDeviceSelection(ui->stdDeviceSelection2, -1);
        return;
    }
    
    // Get the circuit index and check if it's valid
    int circuitIndex = ui->circuitSelection->currentData().toInt();
    if (circuitIndex < 0 || circuitIndex >= circuits.size()) {
        qWarning("Invalid circuit index: %d", circuitIndex);
        return;
    }

    Circuit *circuit = circuits.at(circuitIndex);
    circuit->setDevice1(nullptr);
    circuit->setDevice2(nullptr);

    ui->stdDeviceSelection->setCurrentIndex(0);
    ui->stdDeviceSelection2->setCurrentIndex(0);

    buildStdDeviceSelection(ui->stdDeviceSelection, circuit->getDeviceType(1));
    buildStdDeviceSelection(ui->stdDeviceSelection2, circuit->getDeviceType(2));
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

    for (int i = 0; i < devices.size(); i++) {
        Device *device = devices.at(i);
        if (device->getDeviceType() == type) {
            selection->addItem(device->getName(), i);
        }
    }
}

void ValveWorkbench::buildModelSelection()
{
    qDebug("========== Building Model Selection ==========");
    
    // Clear both device selection combo boxes
    ui->stdDeviceSelection->clear();
    ui->stdDeviceSelection2->clear();
    
    // Add default options
    ui->stdDeviceSelection->addItem("Select...", -1);
    ui->stdDeviceSelection2->addItem("Select...", -1);
    
    // Get the current device type from the UI
    int uiDeviceType = ui->deviceType->currentData().toInt();
    qDebug("Current UI device type: %d", uiDeviceType);
    
    // Enable/disable combo boxes based on device type
    bool showDevice1 = true;
    bool showDevice2 = false;
    
    if (uiDeviceType == TRIODE) {
        // For triodes, we only need Device 1
        showDevice1 = true;
        showDevice2 = false;
    } else if (uiDeviceType == PENTODE) {
        // For pentodes, we only need Device 1
        showDevice1 = true;
        showDevice2 = false;
    } else if (uiDeviceType == DOUBLE_TRIODE) {
        // For double triodes, we need both Device 1 and Device 2
        showDevice1 = true;
        showDevice2 = true;
    }
    
    ui->stdDeviceSelection->setEnabled(showDevice1);
    ui->label_4->setEnabled(showDevice1); // Device 1 label
    
    ui->stdDeviceSelection2->setEnabled(showDevice2);
    ui->label_5->setEnabled(showDevice2); // Device 2 label
    
    // Debug output to show loaded devices
    qDebug("Loaded %lld devices", devices.size());
    
    int triodeCount = 0;
    int pentodeCount = 0;
    
    // Add devices to the appropriate combo boxes based on their model type
    for (int i = 0; i < devices.size(); i++) {
        Device *device = devices.at(i);
        int modelType = device->getDeviceType();
        QString name = device->getName();
        qDebug("Device %d: %s (Model Type: %d)", i, name.toStdString().c_str(), modelType);
        
        // Map model types to UI device types
        if (modelType == MODEL_TRIODE && (uiDeviceType == TRIODE || uiDeviceType == DOUBLE_TRIODE)) {
            // Add triodes when Triode or Double Triode is selected
            ui->stdDeviceSelection->addItem(name, i);
            triodeCount++;
            qDebug("  Added triode '%s' to Device 1", name.toStdString().c_str());
            
            // For double triodes, also add to Device 2
            if (uiDeviceType == DOUBLE_TRIODE) {
                ui->stdDeviceSelection2->addItem(name, i);
                qDebug("  Added triode '%s' to Device 2", name.toStdString().c_str());
            }
        } else if (modelType == MODEL_PENTODE && uiDeviceType == PENTODE) {
            // Add pentodes when Pentode is selected
            ui->stdDeviceSelection->addItem(name, i);
            pentodeCount++;
            qDebug("  Added pentode '%s' to Device 1", name.toStdString().c_str());
        }
    }
    
    qDebug("Added %d triodes and %d pentodes to combo boxes", triodeCount, pentodeCount);
    
    // If no matching devices were loaded, disable the combo boxes
    if (ui->stdDeviceSelection->count() <= 1) { // Only the "Select..." item
        ui->stdDeviceSelection->setEnabled(false);
        qDebug("No matching devices for Device 1, disabled combo box");
    }
    if (ui->stdDeviceSelection2->count() <= 1) { // Only the "Select..." item
        ui->stdDeviceSelection2->setEnabled(false);
        qDebug("No matching devices for Device 2, disabled combo box");
    }
    
    // Check final state of combo boxes
    qDebug("stdDeviceSelection has %d items", ui->stdDeviceSelection->count());
    qDebug("stdDeviceSelection2 has %d items", ui->stdDeviceSelection2->count());
    qDebug("========== Model Selection Built ==========");
}

void ValveWorkbench::selectPlot(int plotType)
{
    (void)plotType; // Parameter currently unused
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
    // Get the circuit index and check if it's valid
    int circuitIndex = ui->circuitSelection->currentData().toInt();
    if (circuitIndex < 0 || circuitIndex >= circuits.size()) {
        qWarning("Invalid circuit index: %d", circuitIndex);
        return;
    }
    
    Circuit *circuit = circuits.at(circuitIndex);
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
    ui->progressBar->setValue(progress);
}

void ValveWorkbench::testFinished()
{
    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);
    ui->btnAddToProject->setEnabled(true);

    currentMeasurement = analyser->getResult();
    measuredCurves = currentMeasurement->updatePlot(&plot);
    plot.add(measuredCurves);
    ui->measureCheck->setChecked(true);
}

void ValveWorkbench::testAborted()
{
    ui->runButton->setChecked(false);
    ui->progressBar->setVisible(false);
}

void ValveWorkbench::checkComPorts() {
    serialPorts = QSerialPortInfo::availablePorts();
    qInfo("Checking for available serial ports...");

    for (const QSerialPortInfo &serialPortInfo : serialPorts) {
        qInfo("Found port: %s, Vendor ID: 0x%04x, Product ID: 0x%04x", 
              serialPortInfo.portName().toStdString().c_str(),
              serialPortInfo.vendorIdentifier(),
              serialPortInfo.productIdentifier());
              
        if (serialPortInfo.vendorIdentifier() == 0x1a86 && serialPortInfo.productIdentifier() == 0x7523) {
            port = serialPortInfo.portName();
            qInfo("Found CH340 USB-Serial adapter on port %s", port.toStdString().c_str());

            setSerialPort(port);
            return;
        }
    }
    
    qInfo("No CH340 USB-Serial adapter found");
    ui->tab_3->setEnabled(false);
}

void ValveWorkbench::setSerialPort(QString portName)
{
    if (serialPort.isOpen()) {
        qInfo("Closing existing serial port connection");
        serialPort.close();
    }

    if (portName == "") {
        qInfo("No port name provided, disabling analyzer tab");
        ui->tab_3->setEnabled(false);
        return;
    }

    qInfo("Setting up serial port %s with baud rate 9600", portName.toStdString().c_str());
    serialPort.setPortName(portName);
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setBaudRate(QSerialPort::Baud9600);
    
    bool opened = serialPort.open(QSerialPort::ReadWrite);
    if (opened) {
        qInfo("Serial port %s opened successfully", portName.toStdString().c_str());
        ui->tab_3->setEnabled(true);
        ui->runButton->setEnabled(true);
        qInfo("Analyzer tab and run button enabled");
    } else {
        qInfo("Failed to open serial port %s: %s", 
              portName.toStdString().c_str(), 
              serialPort.errorString().toStdString().c_str());
        ui->tab_3->setEnabled(false);
        ui->runButton->setEnabled(false);
    }
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
        
        // Load calibration values if they exist
        if (config.contains("calibration") && config["calibration"].isObject()) {
            QJsonObject calibration = config["calibration"].toObject();
            
            // Load voltage calibration values
            if (calibration.contains("heaterVoltage"))
                analyser->setHeaterVoltageCalibration(calibration["heaterVoltage"].toDouble());
            if (calibration.contains("anodeVoltage"))
                analyser->setAnodeVoltageCalibration(calibration["anodeVoltage"].toDouble());
            if (calibration.contains("screenVoltage"))
                analyser->setScreenVoltageCalibration(calibration["screenVoltage"].toDouble());
            if (calibration.contains("gridVoltage"))
                analyser->setGridVoltageCalibration(calibration["gridVoltage"].toDouble());
            
            // Load current calibration values
            if (calibration.contains("heaterCurrent"))
                analyser->setHeaterCurrentCalibration(calibration["heaterCurrent"].toDouble());
            if (calibration.contains("anodeCurrent"))
                analyser->setAnodeCurrentCalibration(calibration["anodeCurrent"].toDouble());
            if (calibration.contains("screenCurrent"))
                analyser->setScreenCurrentCalibration(calibration["screenCurrent"].toDouble());
        }
    }
}

void ValveWorkbench::saveConfig(QString filename)
{
    // Create or update the calibration section in the config
    QJsonObject calibration;
    
    // Store voltage calibration values
    calibration["heaterVoltage"] = analyser->getHeaterVoltageCalibration();
    calibration["anodeVoltage"] = analyser->getAnodeVoltageCalibration();
    calibration["screenVoltage"] = analyser->getScreenVoltageCalibration();
    calibration["gridVoltage"] = analyser->getGridVoltageCalibration();
    
    // Store current calibration values
    calibration["heaterCurrent"] = analyser->getHeaterCurrentCalibration();
    calibration["anodeCurrent"] = analyser->getAnodeCurrentCalibration();
    calibration["screenCurrent"] = analyser->getScreenCurrentCalibration();
    
    // Add calibration to the main config
    config["calibration"] = calibration;
    
    // Save the updated config to file
    QFile configFile(filename);
    if (!configFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open config file for writing.");
        return;
    }
    
    QJsonDocument configDoc(config);
    configFile.write(configDoc.toJson());
    configFile.close();
}

void ValveWorkbench::loadDevices()
{
    // Try several possible locations for the models directory
    QString modelPath;
    QDir modelDir;
    bool found = false;
    
    // Option 1: Direct hardcoded path
    modelPath = "C:\\Users\\lizar\\Documents\\ValveWorkbench\\models";
    modelDir.setPath(modelPath);
    if (modelDir.exists()) {
        found = true;
        qDebug("Found models directory at hardcoded path");
    }
    
    // Option 2: Build directory
    if (!found) {
        modelPath = QCoreApplication::applicationDirPath() + QDir::separator() + "models";
        modelDir.setPath(modelPath);
        if (modelDir.exists()) {
            found = true;
            qDebug("Found models directory in build directory");
        }
    }
    
    // Option 3: Source directory (assuming build is in a subdirectory)
    if (!found) {
        modelPath = QCoreApplication::applicationDirPath();
        QDir appDir(modelPath);
        appDir.cdUp(); // Go up to build directory
        appDir.cdUp(); // Go up to source directory
        modelPath = appDir.absolutePath() + QDir::separator() + "models";
        modelDir.setPath(modelPath);
        if (modelDir.exists()) {
            found = true;
            qDebug("Found models directory in source directory");
            
            // Copy model files from source to build directory for easier access in the future
            QString buildModelPath = QCoreApplication::applicationDirPath() + QDir::separator() + "models";
            QDir buildModelDir(buildModelPath);
            
            // Create the models directory in the build directory if it doesn't exist
            if (!buildModelDir.exists()) {
                QDir buildDir(QCoreApplication::applicationDirPath());
                if (buildDir.mkdir("models")) {
                    qDebug("Created models directory in build directory");
                } else {
                    qDebug("Failed to create models directory in build directory");
                }
            }
            
            // Copy all model files from source to build directory
            QStringList filters;
            filters << "*.vwm" << "*.json";
            modelDir.setNameFilters(filters);
            QStringList modelFiles = modelDir.entryList();
            
            for (const QString& file : modelFiles) {
                QString sourceFile = modelPath + QDir::separator() + file;
                QString destFile = buildModelPath + QDir::separator() + file;
                
                // Only copy if the file doesn't exist or is older than the source
                QFileInfo sourceInfo(sourceFile);
                QFileInfo destInfo(destFile);
                
                if (!destInfo.exists() || sourceInfo.lastModified() > destInfo.lastModified()) {
                    if (QFile::copy(sourceFile, destFile)) {
                        qDebug("Copied model file: %s", file.toStdString().c_str());
                    } else {
                        qDebug("Failed to copy model file: %s", file.toStdString().c_str());
                    }
                }
            }
            
            // Use the build directory for loading models
            modelPath = buildModelPath;
            modelDir.setPath(modelPath);
        }
    }
    
    qDebug("Looking for model files in: %s", modelPath.toStdString().c_str());
    qDebug("Directory exists: %s", modelDir.exists() ? "Yes" : "No");
    
    // Check if the directory exists
    if (!modelDir.exists()) {
        qDebug("ERROR: Models directory does not exist: %s", modelPath.toStdString().c_str());
        return;
    }

    QStringList filters;
    filters << "*.vwm" << "*.json";
    modelDir.setNameFilters(filters);

    QStringList models = modelDir.entryList();
    qDebug("Found %lld model files", models.size());
    for (int i = 0; i < models.size(); i++) {
        qDebug("Model file: %s", models.at(i).toStdString().c_str());
    }

    devices.clear();

    for (int i = 0; i < models.size(); i++) {
        QString modelFile = modelPath + QDir::separator() + models.at(i);
        qDebug("Loading model file: %s", modelFile.toStdString().c_str());
        QFile file(modelFile);

        if (file.open(QIODevice::ReadOnly)) {
            QByteArray modelData = file.readAll();
            QJsonDocument modelDoc = QJsonDocument::fromJson(modelData);
            
            if (modelDoc.isNull() || !modelDoc.isObject()) {
                qDebug("ERROR: Invalid JSON in model file: %s", modelFile.toStdString().c_str());
                continue;
            }
            
            Device *device = new Device(modelDoc);
            int type = device->getDeviceType();
            qDebug("Created device: %s (Type: %d)", device->getName().toStdString().c_str(), type);
            
            // Check if the device type is valid
            if (type != MODEL_TRIODE && type != MODEL_PENTODE) {
                qDebug("WARNING: Invalid device type %d for %s", type, device->getName().toStdString().c_str());
            }
            
            devices.append(device);
        } else {
            qDebug("ERROR: Could not open model file: %s", modelFile.toStdString().c_str());
        }
    }
    
    qDebug("Total devices loaded: %lld", devices.size());
}

void ValveWorkbench::loadTemplate(int index)
{
    // Check if templates list is empty or index is out of range
    if (templates.isEmpty() || index >= templates.size()) {
        // Use default values if no template is available
        ui->deviceName->setText("Default");
        heaterVoltage = 6.3;
        anodeStart = 0.0;
        anodeStop = 300.0;
        anodeStep = 10.0;
        gridStart = 0.0;
        gridStop = -4.0;
        gridStep = -0.5;
        screenStart = 0.0;
        screenStop = 300.0;
        screenStep = 50.0;
        pMax = 1.0;
        iaMax = 5.0;
        
        updateParameterDisplay();
        
        // Set device type to Triode by default
        ui->deviceType->setCurrentIndex(0);
        on_deviceType_currentIndexChanged(0);
        
        // Set test type to Plate Characteristics by default
        ui->testType->setCurrentIndex(0);
        on_testType_currentIndexChanged(0);
        
        return;
    }
    
    Template tpl = templates.at(index);
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
    updateDoubleValue(ui->screenStart, screenStart);
    updateDoubleValue(ui->screenStop, screenStop);
    updateDoubleValue(ui->screenStep, screenStop);
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
    (void)doubleTriode; // Parameter currently unused
    updateParameterDisplay();

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

void ValveWorkbench::handleHeaterTimeout()
{
    analyser->handleHeaterTimeout();
}

void ValveWorkbench::on_stdDeviceSelection_currentIndexChanged(int index)
{
    selectStdDevice(1, ui->stdDeviceSelection->itemData(index).toInt());
}

void ValveWorkbench::on_circuitSelection_currentIndexChanged(int index)
{
    (void)index; // Parameter currently unused
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
    
    // Set current calibration values in the dialog
    preferencesDialog.setHeaterVoltageCalibration(analyser->getHeaterVoltageCalibration());
    preferencesDialog.setAnodeVoltageCalibration(analyser->getAnodeVoltageCalibration());
    preferencesDialog.setScreenVoltageCalibration(analyser->getScreenVoltageCalibration());
    preferencesDialog.setGridVoltageCalibration(analyser->getGridVoltageCalibration());
    
    preferencesDialog.setHeaterCurrentCalibration(analyser->getHeaterCurrentCalibration());
    preferencesDialog.setAnodeCurrentCalibration(analyser->getAnodeCurrentCalibration());
    preferencesDialog.setScreenCurrentCalibration(analyser->getScreenCurrentCalibration());

    if (preferencesDialog.exec() == 1) {
        setSerialPort(preferencesDialog.getPort());

        pentodeModelType = preferencesDialog.getPentodeModelType();

        samplingType = preferencesDialog.getSamplingType();
        
        // Get updated calibration values from the dialog
        analyser->setHeaterVoltageCalibration(preferencesDialog.getHeaterVoltageCalibration());
        analyser->setAnodeVoltageCalibration(preferencesDialog.getAnodeVoltageCalibration());
        analyser->setScreenVoltageCalibration(preferencesDialog.getScreenVoltageCalibration());
        analyser->setGridVoltageCalibration(preferencesDialog.getGridVoltageCalibration());
        
        analyser->setHeaterCurrentCalibration(preferencesDialog.getHeaterCurrentCalibration());
        analyser->setAnodeCurrentCalibration(preferencesDialog.getAnodeCurrentCalibration());
        analyser->setScreenCurrentCalibration(preferencesDialog.getScreenCurrentCalibration());

        // Save the updated configuration
        saveConfig(QCoreApplication::applicationDirPath() + "/analyser.json");

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
    (void)previous; // Parameter currently unused

    void *data = current->data(0, Qt::UserRole).value<void *>();

    bool showScreen = preferencesDialog.showScreenCurrent();

    switch(current->type()) {
    case TYP_PROJECT:
        setSelectedTreeItem(currentProject, false);
        currentProject = current;
        setSelectedTreeItem(currentProject, true);
        setFitButtons();
        ((Project *)data)->updateProperties(ui->properties);
        break;
    case TYP_MEASUREMENT: {
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
            measuredCurves = currentMeasurement->updatePlot(&plot);
            plot.add(measuredCurves);
            modelledCurves = nullptr;
            ui->measureCheck->setChecked(true);
            break;
        }
    case TYP_SWEEP: {
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
                measuredCurves = currentMeasurement->updatePlot(&plot, sweep);
                plot.add(measuredCurves);
                modelledCurves = nullptr;
                ui->measureCheck->setChecked(true);
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
            estimatedCurves = estimate->plotModel(&plot, currentMeasurement);
            plot.add(estimatedCurves);
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

            Sweep *sweep = nullptr;

            if (currentMeasurementItem != nullptr) {
                if (currentMeasurementItem->type() == TYP_SWEEP) {
                    sweep = (Sweep *) currentMeasurementItem->data(0, Qt::UserRole).value<void *>();
                }
            }

            Model *model = (Model *) data;
            model->updateProperties(ui->properties);
            if (currentMeasurement != nullptr) {
                if ((currentMeasurement->getDeviceType() == TRIODE && model->getType() == COHEN_HELIE_TRIODE) || (currentMeasurement->getDeviceType() == PENTODE && model->getType() == GARDINER_PENTODE)) {
                    plot.remove(modelledCurves);
                    model->setShowScreen(showScreen);
                    modelledCurves = model->plotModel(&plot, currentMeasurement, sweep);
                    plot.add(modelledCurves);
                }
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
        triodeMode(index == DOUBLE_TRIODE);
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
    anodeStart = updateVoltage(ui->anodeStart, anodeStart, ANODE);
}

void ValveWorkbench::on_anodeStop_editingFinished()
{
    anodeStop = updateVoltage(ui->anodeStop, anodeStop, ANODE);
}

void ValveWorkbench::on_anodeStep_editingFinished()
{
    anodeStep = updateVoltage(ui->anodeStep, anodeStep, ANODE);
}

void ValveWorkbench::on_gridStart_editingFinished()
{
    gridStart = updateVoltage(ui->gridStart, gridStart, GRID);
}

void ValveWorkbench::on_gridStop_editingFinished()
{
    gridStop = updateVoltage(ui->gridStop, gridStop, GRID);
}

void ValveWorkbench::on_gridStep_editingFinished()
{
    gridStep = updateVoltage(ui->gridStep, gridStep, GRID);
}

void ValveWorkbench::on_screenStart_editingFinished()
{
    screenStart = updateVoltage(ui->screenStart, screenStart, SCREEN);
}

void ValveWorkbench::on_screenStop_editingFinished()
{
    screenStop = updateVoltage(ui->screenStop, screenStop, SCREEN);
}

void ValveWorkbench::on_screenStep_editingFinished()
{
    screenStep = updateVoltage(ui->screenStep, screenStep, SCREEN);
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

void ValveWorkbench::on_heaterButton_clicked()
{
    heaters = !heaters;

    qInfo("Heater state changed to: %s", heaters ? "ON" : "OFF");
    heaterIndicator->setState(heaters);
    ui->runButton->setEnabled(heaters);
    
    // Update heater voltage from the UI
    heaterVoltage = ui->heaterVoltage->text().toDouble();
    qInfo("Setting heater voltage to: %f V", heaterVoltage);
    
    analyser->setHeaterVoltage(heaterVoltage);
    analyser->setIsHeatersOn(heaters);
    
    // Log the run button state
    qInfo("Run button is now %s", ui->runButton->isEnabled() ? "enabled" : "disabled");
}

void ValveWorkbench::on_runButton_clicked()
{
    qInfo("Run button clicked, heaters state: %s", heaters ? "ON" : "OFF");
    
    if (heaters) {
        qInfo("Starting test with parameters:");
        qInfo("  Device type: %d", deviceType);
        qInfo("  Test type: %d", testType);
        qInfo("  Anode: start=%f, stop=%f, step=%f", anodeStart, anodeStop, anodeStep);
        qInfo("  Grid: start=%f, stop=%f, step=%f", gridStart, gridStop, gridStep);
        qInfo("  Screen: start=%f, stop=%f, step=%f", screenStart, screenStop, screenStep);
        qInfo("  Max power: %f W, Max anode current: %f mA", pMax, iaMax);
        
        ui->runButton->setChecked(true);
        ui->progressBar->reset();
        ui->progressBar->setVisible(true);
        ui->btnAddToProject->setEnabled(false);

        analyser->setDeviceType(deviceType);
        analyser->setTestType(testType);
        analyser->setPMax(pMax);
        analyser->setIaMax(iaMax);
        analyser->setSweepParameters(anodeStart, anodeStop, anodeStep, gridStart, gridStop, gridStep, screenStart, screenStop, screenStep);

        qInfo("Calling analyser->startTest()");
        analyser->startTest();
        qInfo("Test started");
    } else {
        qInfo("Cannot start test - heaters are OFF");
        ui->runButton->setChecked(false);
    }
}

void ValveWorkbench::on_btnAddToProject_clicked()
{
    if (currentProject == nullptr) {
        on_actionNew_Project_triggered();
    }

    Project *project = (Project *) currentProject->data(0, Qt::UserRole).value<void *>();
    Measurement *measurement = analyser->getResult();
    if (project->addMeasurement(measurement)) {
        measurement->buildTree(currentProject);
        ui->tabWidget->setCurrentWidget(ui->tab_2);
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
    (void)arg1; // Parameter currently unused
    (void)arg1; // Parameter currently unused
    (void)arg1; // Parameter currently unused
    if (measuredCurves != nullptr) {
        measuredCurves->setVisible(ui->measureCheck->isChecked());
    }
}


void ValveWorkbench::on_modelCheck_stateChanged(int arg1)
{
    (void)arg1; // Parameter currently unused
    (void)arg1; // Parameter currently unused
    (void)arg1; // Parameter currently unused
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

    // Temporarily disabled CompareDialog functionality
    QMessageBox message;
    message.setText("Compare functionality temporarily disabled");
    message.exec();

    // Original code commented out until CompareDialog is implemented
    /*
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
    */
}

