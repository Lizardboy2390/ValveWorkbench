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
#include "../constants.h"
#include "model.h"
#include "simpletriode.h"
#include "korentriode.h"
#include "cohenhelietriode.h"
#include "reefmanpentode.h"
#include "gardinerpentode.h"
#include "../data/measurement.h"

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

    // For pentode devices, expose screen current via the underlying model.
    // Returns current in mA, matching the Model::screenCurrent convention.
    double screenCurrent(double va, double vg1, double vg2);

    // Optional embedded measurement (exported from the analyser) when this
    // Device was created from a preset JSON that included a 'measurement'
    // block. Used by Designer helpers (e.g. SE bias) to reconstruct bias
    // points directly from measured data.
    Measurement *getMeasurement() const { return measurement; }

    // Optional embedded triode seed model (Cohen-Helie) when the preset JSON
    // includes a 'triodeModel' block. Used by Modeller to seed pentode fits
    // without requiring a separate triode model node in the project tree.
    CohenHelieTriode *getTriodeSeed() const { return triodeSeed; }

    // Convenience: try to find a grid bias Vk and screen current Ig2 (both in
    // mA units for Ig2, V for Vk) such that the measured Ia(Va≈vb, Vg1≈-Vk,
    // Vg2≈vs) is close to targetIa_mA. Returns true on success.
    bool findBiasFromMeasurement(double vb,
                                 double vs,
                                 double targetIa_mA,
                                 double &vk_out,
                                 double &ig2_mA_out) const;

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
    void setName(const QString &newName);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

private:
    int deviceType = TRIODE;  // Use main app constants
    int modelType = COHEN_HELIE_TRIODE;

    Model *model = nullptr;

    // Optional measurement attached to this device when loaded from a preset
    // that included analyser sweeps. May be null for legacy presets or
    // hand-authored JSON.
    Measurement *measurement = nullptr;

    // Optional triode seed model used when exporting a fitted pentode: when
    // present, this allows Modeller to reproduce the original triode-based
    // seed for pentode fitting without re-running the triode fit.
    CohenHelieTriode *triodeSeed = nullptr;

    QString name;

    double vaMax;
    double iaMax;
    double vg1Max;
    double vg2Max;
    double ig2Max;
    double paMax;
};
