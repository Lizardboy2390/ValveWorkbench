#pragma once

#include <QTreeWidgetItem>

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

    int getDeviceType() const;
    void setDeviceType(int newDeviceType);
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

    Sweep *at(int i);
    int count();

    bool getShowScreen() const;
    void setShowScreen(bool newShowScreen);

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

    void anodeAxes(Plot *plot);
    QList<QGraphicsItem *> *plotTriodeAnode(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotTriodeTransfer(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotPentodeAnode(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotPentodeTransfer(Plot *plot, Sweep *sweep = nullptr);
    QList<QGraphicsItem *> *plotPentodeScreen(Plot *plot, Sweep *sweep = nullptr);
    double interval(double maxValue);
    void transferAxes(Plot *plot);
    void screenAxes(Plot *plot);
    void propertyEdited(QTableWidgetItem *item);
};

