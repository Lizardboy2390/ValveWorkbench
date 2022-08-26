#pragma once

#include "simpletriode.h"

class KorenTriode : public SimpleTriode
{
public:
    KorenTriode();

	virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0);
	virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination, double vg1Max, double vg2Max = 0);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();

protected:
	void setOptions();
    double korenCurrent(double va, double vg, double kp, double kvb, double a, double mu);
};
