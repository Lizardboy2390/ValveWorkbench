#pragma once

#include "korentriode.h"
#include <vector>

// Using ValveSample structure defined in simpletriode.h

class CohenHelieTriode : public KorenTriode
{
    Q_OBJECT
public:
    CohenHelieTriode();

    double cohenHelieEpk(double v, double vg);

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0);
    virtual double triodeAnodeCurrent(double va, double vg1);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0, bool secondaryEmission = true);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();
    virtual int getType();

    virtual void updateProperties(QTableWidget *properties);

    static double cohenHelieCurrent(double va, double vg, double kg1, double kp, double kvb, double kvb2, double vct, double a, double mu);
    static double cohenHelieEpk(double v, double vg, double kp, double kvb, double kvb2, double vct, double a, double mu);

protected:
	void setOptions();
	// Store measurement samples for direct calculation approach
	std::vector<ValveSample> samples;
};
