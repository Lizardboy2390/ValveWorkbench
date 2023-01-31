#ifndef SAMPLE_H
#define SAMPLE_H

#include <QJsonObject>
#include <QJsonArray>

#include "dataset.h"

class Sample : DataSet
{
public:
    Sample(double va_ = 0.0, double vg1_ = 0.0, double vg2_ = 0.0, double ia_ = 0.0, double ig2_ = 0.0, double vh_ = 0.0, double ih_ = 0.0);

    double getVa() const;
    double getVg1() const;
    double getVg2() const;
    double getIa() const;
    double getIg2() const;
    double getVh() const;
    double getIh() const;

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

    virtual void updateProperties(QTableWidget *properties);

    virtual QGraphicsItemGroup *updatePlot(Plot *plot);

private:
    double vg1 = 0.0;
    double va = 0.0;
    double ia = 0.0;
    double vg2 = 0.0;
    double ig2 = 0.0;
    double vh = 0.0;
    double ih = 0.0;
};

#endif // SAMPLE_H
