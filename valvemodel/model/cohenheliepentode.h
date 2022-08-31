#ifndef COHENHELIEPENTODE_H
#define COHENHELIEPENTODE_H

#include "cohenhelietriode.h"

class CohenHeliePentode : public CohenHelieTriode
{
public:
    CohenHeliePentode(int newType = COHEN_HELIE_PENTODE);

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination, double vg1Max, double vg2Max = 0);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();

    virtual void updateProperties(QTableWidget *properties);

    int getType() const;
    void setType(int newType);

protected:
    int type;
};

#endif // COHENHELIEPENTODE_H
