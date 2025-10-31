#include "project.h"

#include <QJsonArray>

Project::Project()
{
    deviceType = TRIODE;

}

void Project::fromJson(QJsonObject source)
{
    measurements.clear();
    models.clear();
    estimates.clear();

    if (source.contains("name") && source["name"].isString()) {
        name = source["name"].toString();
    }

    if (source.contains("deviceType") && source["deviceType"].isString()) {
        QString sDeviceType = source["deviceType"].toString();
        if (sDeviceType == "pentode") {
            deviceType = PENTODE;
        } else {
            deviceType = TRIODE;
        }
    }

    if (source.contains("measurements") && source["measurements"].isArray()) {
        QJsonArray measurementsArray = source["measurements"].toArray();
        for (int i = 0; i < measurementsArray.size(); i++) {
            if (measurementsArray.at(i).isObject()) {
                Measurement *measurement = new Measurement();
                measurement->fromJson(measurementsArray.at(i).toObject());

                measurements.append(measurement);
            }
        }
    }
}

void Project::toJson(QJsonObject &destination)
{
    QJsonObject projectObject;

    projectObject["name"] = name;

    if (deviceType == TRIODE) {
        projectObject["deviceType"] = "triode";
    } else if (deviceType == PENTODE) {
        projectObject["deviceType"] = "pentode";
    }

    QJsonArray measurementsArray;
    for (int i = 0; i < measurements.size(); i++) {
        QJsonObject measurementObject;
        measurements.at(i)->toJson(measurementObject);

        measurementsArray.append(measurementObject);
    }

    projectObject["measurements"] = measurementsArray;

    destination["project"] = projectObject;
}

QTreeWidgetItem *Project::buildTree(QTreeWidgetItem *parent)
{
    treeItem = parent;

    for (int i = 0; i < measurements.size(); i++) {
        measurements.at(i)->buildTree(treeItem);
    }

    for (int i = 0; i < models.size(); i++) {
        models.at(i)->buildTree(treeItem);
    }

    return treeItem;
}

void Project::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Name", name, 1);
    addProperty(properties, "Type", getTypeName());
    if (deviceType == PENTODE) {
        switch (pentodeType) {
        case PNT_TRUE_PENTODE:
            addProperty(properties, "Pentode Type", "True Pentode");
            addProperty(properties, "Secondary Emission", "No");
            break;
        case PNT_TRUE_PENTODE_SE:
            addProperty(properties, "Pentode Type", "True Pentode");
            addProperty(properties, "Secondary Emission", "Yes");
            break;
        case PNT_BEAM_TETRODE:
            addProperty(properties, "Pentode Type", "Beam Tetrode");
            addProperty(properties, "Secondary Emission", "No");
            break;
        case PNT_BEAM_TETRODE_SE:
            addProperty(properties, "Pentode Type", "Beam Tetrode");
            addProperty(properties, "Secondary Emission", "Yes");
            break;
        }
    }
}

QGraphicsItemGroup *Project::updatePlot(Plot *plot)
{
    return nullptr;
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

const QList<Model *> &Project::getModels() const
{
    return models;
}

int Project::getPentodeType() const
{
    return pentodeType;
}

void Project::setPentodeType(int newPentodeType)
{
    pentodeType = newPentodeType;
}

void Project::setTreeItem(QTreeWidgetItem *item)
{
    treeItem = item;
}

QString Project::getTypeName()
{
    switch(deviceType) {
    case TRIODE:
        return QString("Triode");
        break;
    case PENTODE:
        return QString("Pentode");
        break;
    }

    return QString("Unknown");
}

void Project::propertyEdited(QTableWidgetItem *item)
{
    int field = item->data(Qt::UserRole + 1).value<int>();
    switch (field) {
    case 1:
        name = item->text();
        if (treeItem != nullptr) {
            treeItem->setText(0, name);
        }
        break;
    default:
        break;
    }
}
