#ifndef GARDINERPENTODE_H
#define GARDINERPENTODE_H

#include "cohenhelietriode.h"

class GardinerPentode : public CohenHelieTriode
{
    Q_OBJECT
public:
    GardinerPentode();

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0, bool secondaryEmission = true);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();
    virtual int getType();

    virtual double screenCurrent(double va, double vg1, double vg2, bool secondaryEmission);

    virtual void updateProperties(QTableWidget *properties);


protected:
    void setOptions();
};

#endif // GARDINERPENTODE_H
