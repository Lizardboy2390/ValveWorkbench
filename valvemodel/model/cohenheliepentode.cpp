#include "cohenheliepentode.h"

CohenHeliePentode::CohenHeliePentode(int newType) : type(newType)
{

}

void CohenHeliePentode::addSample(double va, double ia, double vg1, double vg2)
{

}

double CohenHeliePentode::anodeCurrent(double va, double vg1, double vg2)
{
    return 0.0;
}

void CohenHeliePentode::fromJson(QJsonObject source)
{

}

void CohenHeliePentode::toJson(QJsonObject &destination, double vg1Max, double vg2Max)
{

}

void CohenHeliePentode::updateUI(QLabel *labels[], QLineEdit *values[])
{

}

QString CohenHeliePentode::getName()
{
    return "Cohen Helie Pentode";
}

int CohenHeliePentode::getType()
{
    return COHEN_HELIE_PENTODE;
}

void CohenHeliePentode::updateProperties(QTableWidget *properties)
{

}

int CohenHeliePentode::getType() const
{
    return type;
}

void CohenHeliePentode::setType(int newType)
{
    type = newType;
}
