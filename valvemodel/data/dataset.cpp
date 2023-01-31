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

void DataSet::addProperty(QTableWidget *properties, QString label, QString value)
{
    int row = properties->rowCount();
    properties->insertRow(row);
    properties->setItem(row, 0, new QTableWidgetItem(label));
    properties->setItem(row, 1, new QTableWidgetItem(value));
}

QGraphicsItemGroup *DataSet::createGroup(QList<QGraphicsItem *> *segments)
{
    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    for (QGraphicsItem *segment: *segments) {
        group->addToGroup(segment);
    }

    return group;
}
