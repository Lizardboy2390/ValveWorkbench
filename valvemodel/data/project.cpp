#include "project.h"

Project::Project()
{
    deviceType = TRIODE;

}

void Project::fromJson(QJsonObject source)
{

}

void Project::toJson(QJsonObject &destination)
{

}

QTreeWidgetItem *Project::buildTree(QTreeWidgetItem *parent)
{
    treeItem = new QTreeWidgetItem(parent, TYP_PROJECT);

    treeItem->setText(0, "New project");
    treeItem->setIcon(0, QIcon(":/icons/valve32.png"));
    treeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    treeItem->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    for (int i = 0; i < measurements.size(); i++) {
        measurements.at(i)->buildTree(treeItem);
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
