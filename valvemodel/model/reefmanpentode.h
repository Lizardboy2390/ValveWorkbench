#ifndef REEFMANPENTODE_H
#define REEFMANPENTODE_H

#include "cohenhelietriode.h"
#include <vector>

// Sample structure for storing pentode measurement data
#ifndef PENTODE_SAMPLE_DEFINED
#define PENTODE_SAMPLE_DEFINED
struct PentodeSample {
    double va;   // Anode voltage
    double vg1;  // Grid 1 voltage
    double vg2;  // Grid 2 voltage
    double ia;   // Anode current
    double ig2;  // Screen current
};
#endif // PENTODE_SAMPLE_DEFINED

enum eReefmanModelType {
    DERK,
    DERK_E
};

class ReefmanPentode : public CohenHelieTriode
{
    Q_OBJECT
public:
    ReefmanPentode(int newType = DERK);

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0);
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0);
    virtual void fromJson(QJsonObject source);
    virtual void toJson(QJsonObject &destination);
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]);
    virtual QString getName();
    virtual int getType();

    virtual void updateProperties(QTableWidget *properties);

    int getDeviceType() const;
    void setDeviceType(int newType);

    bool withSecondaryEmission() const;
    void setSecondaryEmission(bool newSecondaryEmission);

    int getModelType() const;
    void setModelType(int newModelType);

protected:
    int modelType;
    bool secondaryEmission;

    void setOptions();
    
    // Store measurement samples for direct calculation approach
    std::vector<PentodeSample> anodeSamples;
    std::vector<PentodeSample> screenSamples;
};

#endif // REEFMANPENTODE_H
