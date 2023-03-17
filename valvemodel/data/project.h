#ifndef PROJECT_H
#define PROJECT_H

#include "measurement.h"
#include "../model/estimate.h"
#include "../model/model.h"

class Project : DataSet
{
public:
    Project();

    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);

    virtual QTreeWidgetItem *buildTree(QTreeWidgetItem *parent);

    virtual void updateProperties(QTableWidget *properties);

    virtual QGraphicsItemGroup *updatePlot(Plot *plot);

    int getDeviceType() const;
    void setDeviceType(int newDeviceType);

    const QString &getName() const;
    void setName(const QString &newName);

    bool addMeasurement(Measurement *measurement);
    bool addEstimate(Estimate *estimate);
    bool addModel(Model *model);

    int getPentodeType() const;
    void setPentodeType(int newPentodeType);

    void setTreeItem(QTreeWidgetItem *item);

protected:
    int deviceType;
    int pentodeType;
    QString name;

    QList<Measurement *> measurements;
    QList<Estimate *> estimates;
    QList<Model *> models;

    QString getTypeName();
    void propertyEdited(QTableWidgetItem *item);
};

#endif // PROJECT_H
