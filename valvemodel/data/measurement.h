#pragma once

#include <QTreeWidgetItem>
#include <QColor>

#include "../ui/plot.h"

#include "dataset.h"

class Sample;
class Sweep;

class Measurement : DataSet
{
public:
    Measurement();

    void reset();

    void addSweep(Sweep *sweep);

    void addSample(Sample *sample);

    void nextSweep(double v1Nominal, double v2Nominal = 0.0);

    QList<QGraphicsItem *> *plot(Plot *plot);

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);
    QString measurementName();
    QString deviceName();
    QString testName();

    virtual void updateProperties(QTableWidget *properties);

    virtual QGraphicsItemGroup *updatePlot(Plot *plot);

    QGraphicsItemGroup *updatePlot(Plot *plot, Sweep *sweep);
    QGraphicsItemGroup *updatePlotWithoutAxes(Plot *plot, Sweep *sweep = nullptr);

    int getDeviceType() const;
    void setDeviceType(int newDeviceType);
    bool isTriodeConnectedPentode() const;
    void setTriodeConnectedPentode(bool enable);
    int getTestType() const;
    void setTestType(int newTestType);
    double getHeaterVoltage() const;
    void setHeaterVoltage(double newHeaterVoltage);
    double getAnodeStart() const;
    void setAnodeStart(double newAnodeStart);
    double getAnodeStop() const;
    void setAnodeStop(double newAnodeStop);
    double getAnodeStep() const;
    void setAnodeStep(double newAnodeStep);
    double getGridStart() const;
    void setGridStart(double newGridStart);
    double getGridStop() const;
    void setGridStop(double newGridStop);
    double getGridStep() const;
    void setGridStep(double newGridStep);
    double getScreenStart() const;
    void setScreenStart(double newScreenStart);
    double getScreenStop() const;
    void setScreenStop(double newScreenStop);
    double getScreenStep() const;
    void setScreenStep(double newScreenStep);
    double getIaMax() const;
    void setIaMax(double newIaMax);
    double getPMax() const;
    void setPMax(double newPMax);

    void setSampleColor(const QColor &color);
    QColor getSampleColor() const;

    Sweep *at(int i);
    int count();

    bool getShowScreen() const;
    void setShowScreen(bool newShowScreen);

    bool hasTriodeBData() const;

    void setSmoothPlotting(bool enable);
    bool isSmoothPlotting() const;

protected:
    int deviceType = TRIODE;
    int testType = ANODE_CHARACTERISTICS;

    double heaterVoltage;

    double anodeStart;
    double anodeStop;
    double anodeStep;

    double gridStart;
    double gridStop;
    double gridStep;

    double screenStart;
    double screenStop;
    double screenStep;

    double iaMax;
    double pMax;

    QList<Sweep *> sweeps;

    Sweep *currentSweep;

    bool showScreen = true;
    QColor sampleColor = QColor::fromRgb(0, 0, 0);

    bool smoothPlotting = false;

    // Hint flag: true when this pentode measurement was taken in
    // triode-connected pentode mode (anode and screen driven together).
    bool triodeConnectedPentode = false;

    void anodeAxes(Plot *plot);
    QList<QGraphicsItem *> *plotTriodeAnode(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotTriodeTransfer(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotPentodeAnode(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotPentodeTransfer(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotPentodeScreen(Sweep *sweep = nullptr);
    double interval(double maxValue);
    void transferAxes(Plot *plot);
    void screenAxes(Plot *plot);
    void propertyEdited(QTableWidgetItem *item);
};

