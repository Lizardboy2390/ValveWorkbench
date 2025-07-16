#pragma once

#include <QString>
#include <limits>

class Parameter
{
public:
    Parameter(QString name, double value);

    double getValue() const;
    const QString &getName() const;
    double *getPointer();

    void setValue(double newValue);
    
    // Methods for parameter bounds (replacing Ceres parameter constraints)
    void setLowerBound(double lowerBound);
    void setUpperBound(double upperBound);
    double getLowerBound() const;
    double getUpperBound() const;
    bool isWithinBounds(double testValue) const;

private:
    QString name;
    double value;
    double lowerBound = -std::numeric_limits<double>::max();
    double upperBound = std::numeric_limits<double>::max();
};
