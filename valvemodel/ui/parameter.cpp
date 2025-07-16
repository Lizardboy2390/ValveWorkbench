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
    // Apply bounds when setting value for numerical stability
    if (newValue < lowerBound) {
        value = lowerBound;
    } else if (newValue > upperBound) {
        value = upperBound;
    } else {
        value = newValue;
    }
}

const QString &Parameter::getName() const
{
    return name;
}

double *Parameter::getPointer()
{
    return &value;
}

// Methods for parameter bounds (replacing Ceres parameter constraints)
void Parameter::setLowerBound(double lowerBound)
{
    this->lowerBound = lowerBound;
    
    // Ensure value respects the new bound
    if (value < lowerBound) {
        value = lowerBound;
    }
}

void Parameter::setUpperBound(double upperBound)
{
    this->upperBound = upperBound;
    
    // Ensure value respects the new bound
    if (value > upperBound) {
        value = upperBound;
    }
}

double Parameter::getLowerBound() const
{
    return lowerBound;
}

double Parameter::getUpperBound() const
{
    return upperBound;
}

bool Parameter::isWithinBounds(double testValue) const
{
    return (testValue >= lowerBound && testValue <= upperBound);
}
