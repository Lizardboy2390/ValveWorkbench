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

    virtual QGraphicsItemGroup *updatePlot(Plot *plot) = 0;

    void editCallback(QTableWidgetItem *item);

protected:
    QTreeWidgetItem *treeItem = nullptr;

    void clearProperties(QTableWidget *properties);
    void addProperty(QTableWidget *properties, const QString &label, const QString &valueA, const QString &valueB = QString(), int field = 0);

    virtual void propertyEdited(QTableWidgetItem *item) = 0;

    QGraphicsItemGroup *createGroup(QList<QGraphicsItem *> *segments);
};

