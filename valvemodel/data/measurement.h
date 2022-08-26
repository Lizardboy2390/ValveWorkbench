#pragma once

#include <QTreeWidgetItem>

#include "sweep.h"
#include "../ui/plot.h"

class Measurement : DataSet
{
public:
    Measurement();

    void reset();

    void addSweep(Sweep *sweep);

    void addSample(Sample *sample);

    void nextSweep();

    QList<QGraphicsItem *> *plot(Plot *plot);

    void setDeviceType(int newDeviceType);

    void setTestType(int newTestType);

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);
    QString measurementName();
    QString deviceName();
    QString testName();

    virtual void updateProperties(QTableWidget *properties);

    virtual void updatePlot(Plot *plot);

    int getDeviceType() const;
    int getTestType() const;
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

    void anodeAxes(Plot *plot);
    QList<QGraphicsItem *> *plotTriodeAnode(Plot *plot);
    double interval(double maxValue);
};

