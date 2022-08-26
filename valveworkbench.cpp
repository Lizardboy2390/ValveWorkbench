#include "valveworkbench.h"
#include "ui_valveworkbench.h"

#include "valvemodel/circuit/triodecommoncathode.h"

ValveWorkbench::ValveWorkbench(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ValveWorkbench)
{
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

