#include "parameter.h"

Parameter::Parameter(QString name, double value) : name(name), value(value)
{

}

double Parameter::getValue() const
{
    return value;
}

void Parameter::setValue(double newValue)
{
    value = newValue;
}

const QString &Parameter::getName() const
{
    return name;
}

double *Parameter::getPointer()
{
    return &value;
}
