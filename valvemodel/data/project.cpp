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

    return item;
}

void Project::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);
}

void Project::updatePlot(Plot *plot)
{

}
