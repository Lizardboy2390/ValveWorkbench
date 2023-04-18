#pragma once

#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QGraphicsItemGroup>

#include "ceres/ceres.h"
#include "glog/logging.h"

#include "../ui/parameter.h"
#include "../ui/uibridge.h"
#include "../ui/plot.h"
#include "model.h"
#include "simpletriode.h"
#include "korentriode.h"
#include "cohenhelietriode.h"
#include "reefmanpentode.h"
#include "gardinerpentode.h"

enum eModelDeviceType {
    MODEL_TRIODE,
    MODEL_PENTODE
};

enum ePlotType {
    PLOT_TRIODE_ANODE,
    PLOT_TRIODE_TRANSFER,
    PLOT_PENTODE_ANODE,
    PLOT_PENTODE_TRANSFER,
    PLOT_PENTODE_SCREEN
};

class Device : UIBridge
{
    Q_OBJECT
public:
    Device(int _modelDeviceType);
    Device(QJsonDocument model);

    double getParameter(int index) const;

    double anodeCurrent(double va, double vg1, double vg2 = 0);
    double anodeVoltage(double ia, double vg1, double vg2 = 0);

    void updateUI(QLabel *labels[], QLineEdit *values[]);
    void anodeAxes(Plot *plot);
    void transferAxes(Plot *plot);
    QGraphicsItemGroup *anodePlot(Plot *plot);
    QGraphicsItemGroup *transferPlot(Plot *plot);
    double interval(double maxValue);

    int getModelType() const;
    void setModelType(int newModelType);

    double getVaMax() const;
    double getIaMax() const;
    double getVg1Max() const;
    double getVg2Max() const;
    double getPaMax() const;

    int getDeviceType() const;

    void setDeviceType(int newDeviceType);

    QString getName();

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

private:
    int deviceType = MODEL_TRIODE;
    int modelType = COHEN_HELIE_TRIODE;

    Model *model = nullptr;

    QString name;

    double vaMax;
    double iaMax;
    double vg1Max;
    double vg2Max;
    double ig2Max;
    double paMax;
};
