#ifndef SWEEP_H
#define SWEEP_H

#include <QList>
#include <QTreeWidgetItem>

#include "sample.h"
#include "../constants.h"
#include "../ui/plot.h"

enum eSweepType {
    SWEEP_TRIODE_ANODE,
    SWEEP_PENTODE_ANODE,
    SWEEP_PENTODE_SCREEN
};

class Sweep : DataSet
{
public:
    Sweep(eSweepType type);
    Sweep(int deviceType, int testType);

    void addSample(Sample *sample);
    int count();
    Sample *at(int index);

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);
    QString sweepName();

    virtual void updateProperties(QTableWidget *properties);

    virtual void updatePlot(Plot *plot);

    double getVaNominal() const;
    void setVaNominal(double newVaNominal);
    double getVg1Nominal() const;
    void setVg1Nominal(double newVg1Nominal);
    double getVg2Nominal() const;
    void setVg2Nominal(double newVg2Nominal);

protected:
    QList<Sample *> samples;

    eSweepType type;
    double vaNominal;
    double vg1Nominal;
    double vg2Nominal;
};

#endif // SWEEP_H
