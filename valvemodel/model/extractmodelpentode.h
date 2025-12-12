#pragma once

#include "korentriode.h"

// ExtractModelPentode implements Derk Reefman's "ExtractModel" beam-tetrode / 
// pentode law (DerkE variant) on top of the existing KorenTriode core. The
// underlying idea is:
//
//   1. Use a Koren-like Ip(Vg2, Vg1) law (Ip_Koren) expressed in terms of the
//      screen voltage Vg2 and control grid Vg1, parameterised by
//      {mu, kp, kvb, x}.
//   2. Split Ip_Koren into contributions to the anode and screen via
//      Kg1/Kg2, with additional shaping parameters a, alpha_s, beta to
//      control knee softness and screen partition.
//   3. Optionally add a secondary-emission term Psec(Va) that depends on
//      anode voltage Va and an effective cross-over voltage Vco(Vg2, Vg1)
//      using {omega, lambda, nu, S, ap} as in the ExtractModel papers.
//
// This class owns the full parameter set (Kg1/Kg2, A, alpha_s, beta, and the
// optional secondary-emission geometry) and exposes an anodeCurrent and
// screenCurrent that match the ExtractModel equations used by DerkE. It also
// wires the corresponding Ceres residuals so that anode and screen current
// are fitted simultaneously against Ia and Ig2 data.

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
