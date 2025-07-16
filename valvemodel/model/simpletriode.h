#pragma once

#include "model.h"
#include <vector>

// Sample structure for storing measurement data
struct ValveSample {
    double va;  // Anode voltage
    double ia;  // Anode current
    double vg1; // Grid voltage
};

class SimpleTriode : public Model
{
    Q_OBJECT
public:
    SimpleTriode();

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0);
    virtual double triodeAnodeCurrent(double va, double vg1);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0, bool secondaryEmission = true);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();
    virtual int getType();

    virtual void updateProperties(QTableWidget *properties);

	void setKg(double kg);
	void setMu(double kg);
	void setAlpha(double kg);
	void setVct(double kg);

protected:
	void setOptions();
	
	// Store measurement samples for direct calculation approach
	std::vector<ValveSample> samples;
};
