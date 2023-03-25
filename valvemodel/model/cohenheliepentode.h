#ifndef COHENHELIEPENTODE_H
#define COHENHELIEPENTODE_H

#include "cohenhelietriode.h"

class CohenHeliePentode : public CohenHelieTriode
{
public:
    CohenHeliePentode(int newType = COHEN_HELIE_PENTODE);

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination, double vg1Max, double vg2Max = 0);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();
    virtual int getType();

    virtual void updateProperties(QTableWidget *properties);

    int getDeviceType() const;
    void setDeviceType(int newType);

    bool withSecondaryEmission() const;
    void setSecondaryEmission(bool newSecondaryEmission);

protected:
    int deviceType;
    bool secondaryEmission;

    void setOptions();
};

#endif // COHENHELIEPENTODE_H
