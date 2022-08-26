#include "sample.h"

Sample::Sample(double vg1_, double va_, double ia_, double vg2_, double ig2_, double vh_, double ih_) : vg1(vg1_), va(va_), ia(ia_), vg2(vg2_), ig2(ig2_), vh(vh_), ih(ih_)
{

}

double Sample::getVa() const
{
    return va;
}

double Sample::getVg1() const
{
    return vg1;
}

double Sample::getVg2() const
{
    return vg2;
}

double Sample::getIa() const
{
    return ia;
}

double Sample::getIg2() const
{
    return ig2;
}

double Sample::getVh() const
{
    return vh;
}

double Sample::getIh() const
{
    return ih;
}

void Sample::fromJson(QJsonObject source)
{
    if (source.contains("va") && source["va"].isDouble()) {
        va = source["va"].toDouble();
    }

    if (source.contains("ia") && source["ia"].isDouble()) {
        ia = source["ia"].toDouble();
    }

    if (source.contains("vg1") && source["vg1"].isDouble()) {
        vg1 = source["vg1"].toDouble();
    }

    if (source.contains("vg2") && source["vg2"].isDouble()) {
        vg2 = source["vg2"].toDouble();
    }

    if (source.contains("ig2") && source["ig2"].isDouble()) {
        ig2 = source["ig2"].toDouble();
    }

    if (source.contains("vh") && source["vh"].isDouble()) {
        vh = source["vh"].toDouble();
    }

    if (source.contains("ih") && source["ih"].isDouble()) {
        ih = source["ih"].toDouble();
    }
}

void Sample::toJson(QJsonObject &destination)
{
    destination["va"] = va;
    destination["ia"] = ia;
    destination["vg1"] = vg1;
    destination["vg2"] = vg2;
    destination["ig2"] = ig2;
    destination["vh"] = vh;
    destination["ih"] = ih;
}

QTreeWidgetItem *Sample::buildTree(QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent, TYP_SAMPLE);

    item->setIcon(0, QIcon(":/icons/sample32.png"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant::fromValue((void *) this));

    return item;
}

void Sample::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Va", QString("%1").arg(va));
    addProperty(properties, "Ia", QString("%1").arg(ia));
    addProperty(properties, "Vg1", QString("%1").arg(vg1));
    addProperty(properties, "Vg2", QString("%1").arg(vg2));
    addProperty(properties, "Ig2", QString("%1").arg(ig2));
    addProperty(properties, "Vh", QString("%1").arg(vh));
    addProperty(properties, "Ih", QString("%1").arg(ih));
}

void Sample::updatePlot(Plot *plot)
{

}
