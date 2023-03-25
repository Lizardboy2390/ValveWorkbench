#ifndef REEFMANPENTODE_H
#define REEFMANPENTODE_H

#include "cohenhelietriode.h"

enum eReefmanModelType {
    DERK,
    DERK_E
};

class ReefmanPentode : public CohenHelieTriode
{
public:
    ReefmanPentode(int newType = DERK);

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

    int getModelType() const;
    void setModelType(int newModelType);

protected:
    int modelType;
    bool secondaryEmission;

    void setOptions();
};

#endif // REEFMANPENTODE_H
