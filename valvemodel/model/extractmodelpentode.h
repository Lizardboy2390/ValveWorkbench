#pragma once

#include "korentriode.h"

class ExtractModelPentode : public KorenTriode
{
public:
    ExtractModelPentode();

    virtual void addSample(double va, double ia, double vg1, double vg2 = 0.0, double ig2 = 0.0) override;
    virtual double anodeCurrent(double va, double vg1, double vg2 = 0.0, bool secondaryEmission = true) override;
    virtual double screenCurrent(double va, double vg1, double vg2, bool secondaryEmission = true) override;
    virtual void fromJson(QJsonObject source) override;
    virtual void toJson(QJsonObject &destination) override;
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]) override;
    virtual QString getName() override;
    virtual int getType() override;

    virtual void updateProperties(QTableWidget *properties) override;

protected:
    virtual void setOptions() override;

private:
    static double ipKoren(double vg2, double vg1,
                          double kp, double kvb,
                          double x, double mu);
};
