#pragma once

#include <QMainWindow>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QList>
#include <QJsonDocument>
#include <QTreeWidget>
#include <QSerialPortInfo>
#include <QTimer>
#include <QThread>
#include <QCoreApplication>
#include <QCheckBox>
 #include <memory>
 #include <vector>

class QLabel;
class QGraphicsView;
class QGraphicsTextItem;
class QGraphicsScene;
class TriodeCommonCathode;
class PushPullOutput;

#include "valvemodel/data/project.h"
#include "valvemodel/model/estimate.h"
#include "valvemodel/model/device.h"
#include "valvemodel/model/modelfactory.h"
#include "valvemodel/circuit/circuit.h"
#include "valvemodel/ui/plot.h"
#include "valvemodel/model/template.h"

#include "analyser/analyser.h"
#include "analyser/client.h"

#include "ledindicator/ledindicator.h"
#include "preferencesdialog.h"

class SimpleManualPentodeDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class ValveWorkbench; }
QT_END_NAMESPACE

class ValveWorkbench : public QMainWindow, public Client
{
    Q_OBJECT

public:
    ValveWorkbench(QWidget *parent = nullptr);
    ~ValveWorkbench();

    bool eventFilter(QObject *obj, QEvent *event) override;

    // Methods required to be implemented by the Analyser Client class
    virtual void updateHeater(double vh, double ih);
    virtual void testProgress(int progress);
    virtual void testFinished();
    virtual void testAborted();

public slots:
    void loadModel();
    void modelScreen();
    void remodelAnode();

private slots:
    void on_actionExit_triggered();

    void on_actionPrint_triggered();

    void on_actionOptions_triggered();

    void on_actionLoad_Model_triggered();

    void on_stdDeviceSelection_currentIndexChanged(int index);

    void on_stdDeviceSelection2_currentIndexChanged(int index);

    void on_circuitSelection_currentIndexChanged(int index);

    void on_cir1Value_editingFinished();

    void on_cir2Value_editingFinished();

    void on_cir3Value_editingFinished();

    void on_cir4Value_editingFinished();

    void on_cir5Value_editingFinished();

    void on_cir6Value_editingFinished();

    void on_cir7Value_editingFinished();

    void on_actionNew_Project_triggered();

    void on_projectTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_heaterButton_clicked();

    void on_runButton_clicked();

    void handleReadyRead();

    void handleError(QSerialPort::SerialPortError error);

    void handleTimeout();

    void on_deviceType_currentIndexChanged(int index);

    void on_testType_currentIndexChanged(int index);

    void on_anodeStart_editingFinished();

    void on_anodeStop_editingFinished();

    void on_anodeStep_editingFinished();

    void on_gridStart_editingFinished();

    void on_gridStop_editingFinished();

    void on_gridStep_editingFinished();

    void on_screenStart_editingFinished();

    void on_screenStop_editingFinished();

    void on_screenStep_editingFinished();

    void on_iaMax_editingFinished();

    void on_pMax_editingFinished();

    void on_btnAddToProject_clicked();

    void importFromDevice();

    void on_fitTriodeButton_clicked();

    void on_fitPentodeButton_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_measureCheck_stateChanged(int arg1);

    void on_modelCheck_stateChanged(int arg1);
    void on_screenCheck_stateChanged(int arg1);
    void on_designerCheck_stateChanged(int arg1);
    void on_autoscaleYCheck_stateChanged(int arg1);
    void on_symSwingCheck_stateChanged(int arg1);
    void on_inputSensitivityCheck_stateChanged(int arg1);
    void on_useBypassedGainCheck_stateChanged(int arg1);
    void on_inductiveLoadCheck_stateChanged(int arg1);

    void on_properties_itemChanged(QTableWidgetItem *item);

    void on_actionSave_Project_triggered();

    void on_actionOpen_Project_triggered();

    void on_actionClose_Project_triggered();

    void on_compareButton_clicked();

    void on_cir8Value_editingFinished();

    void on_cir9Value_editingFinished();

    void on_cir10Value_editingFinished();

    void on_cir11Value_editingFinished();

    void on_cir12Value_editingFinished();

    void on_cir13Value_editingFinished();

    void on_actionExport_Model_triggered();

    void exportFittedModelToDevices();

    // File → Export to Spice: export the currently selected Designer device's
    // tube model as a SPICE .subckt file using the same SPICE helper used by
    // Export-to-Devices. This writes a tube-only netlist that external
    // simulators (ngspice/LTspice/etc.) can include.
    void on_actionExport_to_Spice_triggered();

    void on_actionExport_SE_Output_to_Spice_triggered();

    // Modeller small-signal source toggle: measured (unchecked) vs model (checked)
    void on_mes_mod_select_stateChanged(int state);

    // Analyser tab: Template buttons (Load/Save Template)
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();

    void on_quickHealthButton_clicked();
    void on_fullHealthButton_clicked();

    void on_modellingTestsButton_clicked();
    void on_processModellingTestsButton_clicked();

    void on_actionSave_as_Reference_Tube_triggered();
    void on_actionReset_Reference_Tube_triggered();

    void on_datasheetVa_editingFinished();
    void on_datasheetVg_editingFinished();
    void on_datasheetVg2_editingFinished();
    void on_datasheetIa_editingFinished();
    void on_datasheetGm_editingFinished();
    void on_datasheetMu_editingFinished();
    void on_datasheetRp_editingFinished();
    void on_datasheetIg2_editingFinished();
    void on_datasheetPg2_editingFinished();

    void clearModellerOpMarker();
    void updateModellerOpMarker();
    bool computeModellerOperatingPoint(double &xOp, double &iaOp_mA);

private:
    enum HealthMode {
        HEALTH_NONE,
        HEALTH_QUICK,
        HEALTH_FULL
    };

    struct HealthPoint {
        double va;
        double vg;
        double vg2;
    };

    struct HealthResult {
        bool valid;
        double va;
        double vg;
        double vg2;
        double ia;
        double gm;
        double rp;   // Measured plate resistance (ohms); 0.0 if unavailable
        double ig2;  // Screen current (mA); 0.0 if unavailable
    };

    struct ModellingTestStep {
        QString label;
        int deviceType;
        int testType;
        bool triodeConnectedPentode;

        double anodeStart;
        double anodeStop;
        double anodeStep;

        double gridStart;
        double gridStop;
        double gridStep;

        double screenStart;
        double screenStop;
        double screenStep;
    };

    Ui::ValveWorkbench *ui;
    QCheckBox *designerCheck = nullptr;
    QCheckBox *symSwingCheck = nullptr;
    QCheckBox *inputSensitivityCheck = nullptr;
    QCheckBox *useBypassedGainCheck = nullptr;

    // UI related member variables
    QLineEdit *circuitValues[16];
    QLabel *circuitLabels[16];
    QTableWidget *dataTable;

    Plot plot;
    QGraphicsItemGroup *modelPlot = nullptr;

    QGraphicsTextItem *cursorLabelItem = nullptr;

    QGraphicsScene *headroomWaveformScene = nullptr;

    // Non-UI related member variables
    QList<Device *> devices;
    Device *currentDevice = nullptr;
    Device *customDevice = nullptr;

    QTreeWidgetItem *currentProject = nullptr;

    QList<Circuit *> circuits;
    QTreeWidgetItem *projectTree;

    int deviceType = TRIODE;
    int testType = ANODE_CHARACTERISTICS;
    int pentodeModelType = GARDINER_PENTODE;
    int samplingType = 0;

    double heaterVoltage;
    int badRetryCount = 0;

    double anodeStart;
    double anodeStop;
    double anodeStep;

    double gridStart;
    double gridStop;
    double gridStep;

    double screenStart;
    double screenStop;
    double screenStep;

    double secondGridStart;
    double secondGridStop;
    double secondGridStep;

    double secondAnodeStart;
    double secondAnodeStop;
    double secondAnodeStep;

    double iaMax;
    double pMax;

    bool isDoubleTriode = false;
    bool isTriodeConnectedPentode = false;

    QFile *logFile;

    LedIndicator *heaterIndicator;

    QGraphicsItemGroup *measuredCurves = nullptr;
    QGraphicsItemGroup *measuredCurvesSecondary = nullptr;
    QGraphicsItemGroup *estimatedCurves = nullptr;
    QGraphicsItemGroup *modelledCurves = nullptr;
    QGraphicsItemGroup *modelledCurvesSecondary = nullptr;
    QGraphicsItemGroup *modellerOpMarker = nullptr;

    // Harmonics tab UI elements (created programmatically)
    QWidget *harmonicsTab = nullptr;
    QPushButton *harmonicsRunButton = nullptr;
    QPushButton *harmonicsBiasSweepButton = nullptr;
    QPushButton *harmonicsHeatmapButton = nullptr;
    QPushButton *harmonicsWaterfallButton = nullptr;
    class QTextEdit *harmonicsText = nullptr;
    Plot harmonicsPlot;
    class QGraphicsView *harmonicsView = nullptr;
    
    // 3D rotation controls
    class QSlider *harmonicsRotationXSlider = nullptr;
    class QSlider *harmonicsRotationYSlider = nullptr;

    // Per-tab overlay state for measurement, model, and screen visibility.
    // Logical roles: 0 = Designer, 1 = Modeller, 2 = Analyser.
    struct TabOverlayState {
        bool showMeasurement;
        bool showModel;
        bool showScreen;
    };
    TabOverlayState overlayStates[3];

    Measurement *currentMeasurement = nullptr;
    Sweep *currentSweep = nullptr;
    QTreeWidgetItem *currentMeasurementItem = nullptr;
    QTreeWidgetItem *currentEstimateItem = nullptr;
    QTreeWidgetItem *currentModelItem = nullptr;

    Model *model;
    Model *triodeModelPrimary = nullptr;
    Model *triodeModelSecondary = nullptr;
    Measurement *triodeMeasurementPrimary = nullptr;
    Measurement *triodeMeasurementSecondary = nullptr;
    std::vector<std::unique_ptr<Measurement>> processModellingTestsBinnedTransfers;
    QList<Measurement *> triodeBClones;
    bool triodeBFitPending = false;
    bool runningTriodeBFit = false;
    bool autoTriodeFitRun = false;

    QThread *thread = nullptr;
    QTreeWidgetItem *modelProject = nullptr;
    bool doPentodeModel = false;

    Analyser *analyser;
    QString port;
    QSerialPort serialPort;

    QTimer timeoutTimer;

    QList<QSerialPortInfo> serialPorts;

    QJsonObject config;
    QList<Template> templates;
    QJsonObject datasheetJson; // Opaque datasheet/ref-point block from templates/devices
    QJsonObject analyserTestsDefaults; // Per-test analyser ranges/limits loaded from analyserDefaults.tests

    HealthMode healthMode = HEALTH_NONE;
    bool healthRunActive = false;
    int healthRunIndex = 0;
    QList<HealthPoint> healthPoints;
    QList<HealthResult> healthResults;

    bool healthPrereqAnodeSweepActive = false;
    Measurement *healthPrereqAnodeMeasurement = nullptr;

    bool healthOpFinderActive = false;
    double healthOpTargetIa_mA = 0.0;

    bool healthStateSaved = false;
    int savedTestTypeForHealth = 0;
    double savedAnodeStartForHealth = 0.0;
    double savedAnodeStopForHealth = 0.0;
    double savedAnodeStepForHealth = 0.0;
    double savedGridStartForHealth = 0.0;
    double savedGridStopForHealth = 0.0;
    double savedGridStepForHealth = 0.0;
    double savedScreenStartForHealth = 0.0;
    double savedScreenStopForHealth = 0.0;
    double savedScreenStepForHealth = 0.0;

    bool modellingRunActive = false;
    int modellingRunIndex = 0;
    QList<ModellingTestStep> modellingSteps;

    bool modellingStateSaved = false;
    int savedDeviceTypeForModelling = 0;
    bool savedIsTriodeConnectedForModelling = false;
    int savedTestTypeForModelling = 0;
    double savedAnodeStartForModelling = 0.0;
    double savedAnodeStopForModelling = 0.0;
    double savedAnodeStepForModelling = 0.0;
    double savedGridStartForModelling = 0.0;
    double savedGridStopForModelling = 0.0;
    double savedGridStepForModelling = 0.0;
    double savedScreenStartForModelling = 0.0;
    double savedScreenStopForModelling = 0.0;
    double savedScreenStepForModelling = 0.0;
    double savedIaMaxForModelling = 0.0;
    double savedPMaxForModelling = 0.0;

    PreferencesDialog preferencesDialog;

    SimpleManualPentodeDialog *simplePentodeDialog = nullptr;

    void checkComPorts();
    void setSerialPort(QString portName);

    void readConfig(QString filename);

    void loadDevices();

    //void buildModelSelection();

    // Compute small-signal gm, ra and mu from the currently selected
    // measurement at an automatically chosen operating point. This is used
    // by the Modeller tab for tube matching when in measured mode
    // (mes_mod_select unchecked).
    void updateSmallSignalFromMeasurement(Measurement *measurement);
    // Compute small-signal gm, ra and mu from the current fitted model at an
    // operating point derived from the active measurement. This is used in
    // model mode (mes_mod_select checked) when a Designer circuit does not
    // provide small-signal values directly (e.g. pentode models).
    void updateSmallSignalFromModel(Model *modelForSmallSignal, Measurement *measurement);
    void buildCircuitParameters();
    void buildCircuitSelection();
    void buildStdDeviceSelection(QComboBox *selection, int type);
    void selectStdDevice(int index, int device);
    void selectStdModel(int model);
    void selectDevice(int deviceType);
    void selectModel(int modelType);
    void selectCircuit(int circuitType);
    void selectPlot(int plotType);
    void plotModel();
    double checkDoubleValue(QLineEdit *input, double oldValue);
    void updateDoubleValue(QLineEdit *input, double value);
    void updateCircuitParameter(int index);
    void loadTemplate(int index);
    void saveSamples(QString filename);
    void pentodeMode();
    void triodeMode(bool doubleTriode);
    void diodeMode();
    void log(QString message);
    double updateVoltage(QLineEdit *input, double oldValue, int electrode);
    double updatePMax();
    double updateIaMax();
    void updateParameterDisplay();
    void plotCurrentModelOverMeasurement();
    void ensureSimplePentodeDialog();
    void doPlot();
    QTreeWidgetItem *getProject(QTreeWidgetItem *current);
    QTreeWidgetItem *getParent(QTreeWidgetItem *current, int type);
    Model *findModel(int type);
    Measurement *findMeasurement(int deviceType, int measurementType);
    void setSelectedTreeItem(QTreeWidgetItem *item, bool selected);
    void setFitButtons();
    void modelTriode();
    void modelPentode();
    bool measurementHasTriodeBData(Measurement *measurement) const;
    Measurement *createTriodeBMeasurementClone(Measurement *source) const;
    void deleteMeasurementClone(Measurement *measurement) const;
    void cleanupTriodeBResources();
    void startTriodeBFit();
    void finalizeTriodeModelling();
    void applyTriodeBProperties(Model *primary, Model *secondary);
    Measurement *firstTriodeBMeasurement() const;
    void queueTriodeModelRun(Model *modelToRun);
    bool measurementHasValidSamples(Measurement *measurement) const;
    void populateDataTableFromMeasurement(Measurement *measurement);

    void updateDatasheetDisplay();
    void syncDatasheetFromUi();
    bool ensureDatasheetRefPoint(double &va0, double &vg0, double &ia0, double &gm0, double &mu0, double &rp0);
    bool ensureDatasheetRefPointPentode(double &va0, double &vg0, double &vg20, double &ia0, double &gm0);
    void startHealthRun(HealthMode mode);
    void configureTransferForHealthPoint(const HealthPoint &pt);
    bool findPentodeHealthOperatingPoint(Measurement *measurement, double targetIa_mA, HealthPoint &outPt, double &outIa_mA) const;
    bool computeIaGmAt(Measurement *measurement, const HealthPoint &pt, double &ia_mA, double &gm_mA_V, double &rp_ohms, double *ig2_mA = nullptr);
    void finalizeHealthRun();

    bool captureHealthReferenceFromLastRun();

    void applyModellingStep(const ModellingTestStep &step);
    void restoreModellingState();

    QList<Measurement *> collectModellingTestMeasurements(QTreeWidgetItem *projectItem) const;

    void runHarmonicsScan();
    void runHarmonicsBiasSweep();
    void runHarmonicsHeatmap();
    void runHarmonicsWaterfall();
    void runHarmonicsClippingAnalysis();
    void onHarmonicsRotationChanged(); // Handle 3D rotation slider changes
    void hideRotationControls(); // Hide rotation controls for non-3D plots
    void refreshHarmonicsPlots();
    void updateHeadroomWaveformView(TriodeCommonCathode *tcc);
    void updateHeadroomWaveformView(class SingleEndedOutput *se);
    void updateHeadroomWaveformView(class PushPullOutput *pp);
};
