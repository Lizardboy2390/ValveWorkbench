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
    properties->setColumnCount(2);
}

void DataSet::addProperty(QTableWidget *properties, const QString &label, const QString &valueA, const QString &valueB, int field)
{
    int row = properties->rowCount();
    properties->insertRow(row);

    QTableWidgetItem *labelItem = new QTableWidgetItem(label);
    labelItem->setFlags(Qt::ItemIsEnabled);
    properties->setItem(row, 0, labelItem);
    QTableWidgetItem *valueItem = new QTableWidgetItem(valueA);
    if (field > 0) {
        valueItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
        valueItem->setData(Qt::UserRole, QVariant::fromValue((DataSet *) this));
        valueItem->setData(Qt::UserRole + 1, QVariant::fromValue(field));
    } else {
        valueItem->setFlags(Qt::ItemIsEnabled);
    }
    properties->setItem(row, 1, valueItem);

    if (!valueB.isNull()) {
        if (properties->columnCount() < 3) {
            properties->setColumnCount(3);
        }

        QTableWidgetItem *valueBItem = new QTableWidgetItem(valueB);
        valueBItem->setFlags(Qt::ItemIsEnabled);
        properties->setItem(row, 2, valueBItem);
    } else if (properties->columnCount() >= 3) {
        QTableWidgetItem *valueBItem = new QTableWidgetItem(QString());
        valueBItem->setFlags(Qt::ItemIsEnabled);
        properties->setItem(row, 2, valueBItem);
    }
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
