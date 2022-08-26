#pragma once

#include "model.h"

class SimpleTriode : public Model
{
public:
    SimpleTriode();

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
};
