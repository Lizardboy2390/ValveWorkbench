#ifndef GARDINERPENTODE_H
#define GARDINERPENTODE_H

#include "cohenhelietriode.h"
#include <vector>

// Forward declaration from reefmanpentode.h
#ifndef PENTODE_SAMPLE_DEFINED
#define PENTODE_SAMPLE_DEFINED
struct PentodeSample {
    double va;   // Anode voltage
    double vg1;  // Grid voltage
    double ia;   // Anode current
    double vg2;  // Screen voltage
    double ig2;  // Screen current
};
#endif // PENTODE_SAMPLE_DEFINED

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
    
    // Store measurement samples for direct calculation approach
    std::vector<PentodeSample> anodeSamples;
    std::vector<PentodeSample> screenSamples;
    std::vector<PentodeSample> anodeRemodelSamples;
};

#endif // GARDINERPENTODE_H
