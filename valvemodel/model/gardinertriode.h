#ifndef GARDINERTRIODE_H
#define GARDINERTRIODE_H

#include "model.h"

class GardinerTriode : public Model
{
public:
    GardinerTriode();

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination, double vg1Max, double vg2Max = 0);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();

    void setKg(double kg);
    void setMu(double kg);
    void setAlpha(double kg);
    void setVct(double kg);

protected:
    void setOptions();
    double gardinerCurrent(double va, double vg, double kg, double kvb, double kvb2, double vct, double a, double mu);
};

#endif // GARDINERTRIODE_H
