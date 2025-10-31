#include "project.h"
#include <QJsonArray>

Project::Project()
    : deviceType(0), treeItem(nullptr)
{
}

Project::~Project()
{
    // Clean up if needed
}

void Project::setName(const QString &name)
{
    this->name = name;
}

const QString &Project::getName() const
{
    return name;
}

void Project::setDeviceType(int type)
{
    deviceType = type;
}

int Project::getDeviceType() const
{
    return deviceType;
}

bool Project::addMeasurement(Measurement *measurement)
{
    measurements.append(measurement);
    return true;
}

bool Project::addModel(Model *model)
{
    models.append(model);
    return true;
}

const QList<Model *> &Project::getModels() const
{
    return models;
}

void Project::setTreeItem(QTreeWidgetItem *item)
{
    treeItem = item;
}

QTreeWidgetItem *Project::getTreeItem() const
{
    return treeItem;
}

void Project::toJson(QJsonObject &object) const
{
    object["name"] = name;
    object["deviceType"] = deviceType;
}

void Project::fromJson(const QJsonObject &object)
{
    name = object["name"].toString();
    deviceType = object["deviceType"].toInt();
}

void Project::updateProperties(QWidget *properties)
{
    // Implementation for updating properties UI
}
