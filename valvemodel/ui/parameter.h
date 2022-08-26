#pragma once

#include <QString>

class Parameter
{
public:
    Parameter(QString name, double value);

    double getValue() const;
    const QString &getName() const;
    double *getPointer();

    void setValue(double newValue);

private:
    QString name;
    double value;
};
