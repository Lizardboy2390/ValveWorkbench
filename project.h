#ifndef APP_PROJECT_H
#define APP_PROJECT_H

#include <QString>
#include <QJsonObject>
#include <QTreeWidgetItem>
#include <QList>
#include "valvemodel/data/measurement.h"
#include "valvemodel/model/model.h"

class Project
{
public:
    Project();
    ~Project();

    void setName(const QString &name);
    const QString &getName() const;

    void setDeviceType(int type);
    int getDeviceType() const;

    bool addMeasurement(Measurement *measurement);
    bool addModel(Model *model);

    const QList<Model *> &getModels() const;

    void setTreeItem(QTreeWidgetItem *item);
    QTreeWidgetItem *getTreeItem() const;

    void toJson(QJsonObject &object) const;
    void fromJson(const QJsonObject &object);

    void updateProperties(QWidget *properties);

private:
    QString name;
    int deviceType;
    QList<Measurement *> measurements;
    QList<Model *> models;
    QTreeWidgetItem *treeItem;
};

#endif // APP_PROJECT_H
