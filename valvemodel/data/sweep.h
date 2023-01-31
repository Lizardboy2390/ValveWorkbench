#ifndef SWEEP_H
#define SWEEP_H

#include <QList>
#include <QTreeWidgetItem>

#include "sample.h"
#include "../constants.h"
#include "../ui/plot.h"

class Measurement;

enum eSweepType {
    SWEEP_TRIODE_ANODE,
    SWEEP_TRIODE_GRID,
    SWEEP_PENTODE_ANODE,
    SWEEP_PENTODE_GRID,
    SWEEP_PENTODE_SCREEN
};

class Sweep : DataSet
{
public:
    Sweep(eSweepType type);
    Sweep(int deviceType, int testType, double v1Nominal = 0.0, double v2Nominal = 0.0);

    void addSample(Sample *sample);
    int count();
    Sample *at(int index);

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);
    QString sweepName();

    virtual void updateProperties(QTableWidget *properties);

    virtual QGraphicsItemGroup *updatePlot(Plot *plot);

    void plotTriodeAnode(Plot *plot, QPen *samplePen, QList<QGraphicsItem *> *segments);

    double getVaNominal() const;
    void setVaNominal(double newVaNominal);
    double getVg1Nominal() const;
    void setVg1Nominal(double newVg1Nominal);
    double getVg2Nominal() const;
    void setVg2Nominal(double newVg2Nominal);

    void setMeasurement(Measurement *measurement);

    Measurement *getMeasurement() const;

protected:
    QList<Sample *> samples;

    eSweepType type;
    double vaNominal;
    double vg1Nominal;
    double vg2Nominal;

    Measurement *measurement = nullptr;
};

#endif // SWEEP_H
