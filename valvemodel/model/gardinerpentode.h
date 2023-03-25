#ifndef GARDINERPENTODE_H
#define GARDINERPENTODE_H

#include "cohenhelietriode.h"

class GardinerPentode : public CohenHelieTriode
{
public:
    GardinerPentode();

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination, double vg1Max, double vg2Max = 0);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();
    virtual int getType();

    virtual void updateProperties(QTableWidget *properties);

    bool withSecondaryEmission() const;
    void setSecondaryEmission(bool newSecondaryEmission);

protected:
    bool secondaryEmission;

    void setOptions();
};

#endif // GARDINERPENTODE_H
