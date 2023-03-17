#include "dataset.h"

DataSet::DataSet()
{

}

void DataSet::clearProperties(QTableWidget *properties)
{
    int count = properties->rowCount();
    for (int i = 0; i < count; i++) {
        properties->removeRow(0);
    }
}

void DataSet::addProperty(QTableWidget *properties, QString label, QString value, int field)
{
    int row = properties->rowCount();
    properties->insertRow(row);

    QTableWidgetItem *labelItem = new QTableWidgetItem(label);
    labelItem->setFlags(Qt::ItemIsEnabled);
    properties->setItem(row, 0, labelItem);
    QTableWidgetItem *valueItem = new QTableWidgetItem(value);
    if (field > 0) {
        valueItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
        valueItem->setData(Qt::UserRole, QVariant::fromValue((DataSet *) this));
        valueItem->setData(Qt::UserRole + 1, QVariant::fromValue(field));
    } else {
        valueItem->setFlags(Qt::ItemIsEnabled);
    }
    properties->setItem(row, 1, valueItem);
}

void DataSet::editCallback(QTableWidgetItem *item)
{
    propertyEdited(item);
}

QGraphicsItemGroup *DataSet::createGroup(QList<QGraphicsItem *> *segments)
{
    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    for (QGraphicsItem *segment: *segments) {
        group->addToGroup(segment);
    }

    return group;
}
