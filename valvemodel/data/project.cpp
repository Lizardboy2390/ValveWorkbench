#include "project.h"

Project::Project()
{

}

void Project::fromJson(QJsonObject source)
{

}

void Project::toJson(QJsonObject &destination)
{

}

QTreeWidgetItem *Project::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_SWEEP);

    item->setText(0, "New project");
    item->setIcon(0, QIcon(":/icons/valve32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    for (int i = 0; i < measurements.size(); i++) {
        measurements.at(i)->buildTree(item);
    }

    return item;
}

void Project::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);
}

void Project::updatePlot(Plot *plot)
{

}

int Project::getDeviceType() const
{
    return deviceType;
}

void Project::setDeviceType(int newDeviceType)
{
    deviceType = newDeviceType;
}

const QString &Project::getName() const
{
    return name;
}

void Project::setName(const QString &newName)
{
    name = newName;
}

bool Project::addMeasurement(Measurement *measurement)
{
    if (measurements.contains(measurement)) {
        return false;
    }

    measurements.append(measurement);
    return true;
}

bool Project::addEstimate(Estimate *estimate)
{
    if (estimates.contains(estimate)) {
        return false;
    }

    estimates.append(estimate);
    return true;
}

bool Project::addModel(Model *model)
{
    if (models.contains(model)) {
        return false;
    }

    models.append(model);
    return true;
}
