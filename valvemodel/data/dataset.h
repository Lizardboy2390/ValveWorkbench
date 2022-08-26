#pragma once

#include <QJsonObject>
#include <QTreeWidgetItem>
#include <QTableWidget>

#include "../constants.h"
#include "../ui/plot.h"

class DataSet
{
public:
    DataSet();

    virtual void fromJson(QJsonObject source) = 0;
    virtual void toJson(QJsonObject &destination) = 0;

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent) = 0;

    virtual void updateProperties(QTableWidget *properties) = 0;

    virtual void updatePlot(Plot *plot) = 0;

protected:
    void clearProperties(QTableWidget *properties);
    void addProperty(QTableWidget *properties, QString label, QString value);
};

