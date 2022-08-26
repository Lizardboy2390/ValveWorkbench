#ifndef PROJECT_H
#define PROJECT_H

#include "measurement.h"

class Project : DataSet
{
public:
    Project();

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

    virtual void updateProperties(QTableWidget *properties);

    virtual void updatePlot(Plot *plot);

protected:
    int deviceType;
    QString name;
};

#endif // PROJECT_H
