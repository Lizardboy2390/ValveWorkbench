#pragma once

#include <QMainWindow>
#include <QFileDialog>
#include <QDir>
#include <QList>
#include <QJsonDocument>
#include <QTreeWidget>

#include "valvemodel/data/measurement.h"
#include "valvemodel/model/device.h"
#include "valvemodel/circuit/circuit.h"
#include "valvemodel/ui/plot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ValveWorkbench; }
QT_END_NAMESPACE

class ValveWorkbench : public QMainWindow
{
    Q_OBJECT

public:
    ValveWorkbench(QWidget *parent = nullptr);
    ~ValveWorkbench();

private slots:
    void on_actionExit_triggered();

    void on_actionPrint_triggered();

    void on_actionLoad_Model_triggered();

    void on_stdDeviceSelection_currentIndexChanged(int index);

    void on_stdModelSelection_currentIndexChanged(int index);

    void on_circuitSelection_currentIndexChanged(int index);

    void on_cir1Value_editingFinished();

    void on_cir2Value_editingFinished();

    void on_cir3Value_editingFinished();

    void on_cir4Value_editingFinished();

    void on_cir5Value_editingFinished();

    void on_cir6Value_editingFinished();

    void on_cir7Value_editingFinished();

    void on_actionNew_Project_triggered();

    void on_actionLoad_Measurement_triggered();

    void on_projectTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    Ui::ValveWorkbench *ui;

    // UI related member variables
    QLineEdit *circuitValues[8];
    QLabel *circuitLabels[8];

    Plot plot;
    QGraphicsItemGroup *modelPlot = nullptr;

    // Non-UI related member variables
    QList<Device *> devices;
    Device* currentDevice = nullptr;
    Device* customDevice = nullptr;

    QList<Circuit *> circuits;
    QTreeWidgetItem *projectTree;

    void buildModelSelection();
    void buildCircuitParameters();
    void buildCircuitSelection();
    void buildStdDeviceSelection();
    void selectStdDevice(int device);
    void selectStdModel(int model);
    void selectDevice(int deviceType);
    void selectModel(int modelType);
    void selectCircuit(int circuitType);
    void selectPlot(int plotType);
    void plotModel();
    double checkDoubleValue(QLineEdit *input, double oldValue);
    void updateDoubleValue(QLineEdit *input, double value);
    void updateCircuitParameter(int index);
};
