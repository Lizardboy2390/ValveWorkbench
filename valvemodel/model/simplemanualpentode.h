#ifndef SIMPLEMANUALPENTODE_H
#define SIMPLEMANUALPENTODE_H

#include "cohenhelietriode.h"

// SimpleManualPentode: manual, slider-driven pentode model used on the
// Modeller tab when the user selects the "Simple Manual Pentode" mode.
// It reuses the Cohen-Helie epk core but exposes a compact set of
// Gardiner/Reefman-style shaping parameters (mu, kp, kg1, kg2, alpha,
// beta, gamma, a, etc.) that can be adjusted directly in the UI.
// Unlike the Ceres-based pentode models, this class does not add any
// residuals to the solver; `anodeCurrent` is evaluated directly from
// the parameters and measurement data is used only for seeding.

class SimpleManualPentode : public CohenHelieTriode
{
public:
    SimpleManualPentode();

    // Pentode path: use 3-arg anodeCurrent; triodeAnodeCurrent is
    // inherited from CohenHelieTriode and unused here.
    virtual void addSample(double va, double ia,
                           double vg1, double vg2 = 0.0, double ig2 = 0.0) override;
    virtual double anodeCurrent(double va, double vg1,
                                double vg2 = 0.0, bool secondaryEmission = true) override;
    virtual void fromJson(QJsonObject source) override;
    virtual void toJson(QJsonObject &destination) override;
    virtual void updateUI(QLabel *labels[], QLineEdit *values[]) override;
    virtual QString getName() override;
    virtual int getType() override;
    virtual void updateProperties(QTableWidget *properties) override;

    // Helper for UI layers (e.g. SimpleManualPentodeDialog) to access Parameter objects
    Parameter *getParameterObject(int index) { return parameter[index]; }

protected:
    void setOptions() override;
};

#endif // SIMPLEMANUALPENTODE_H
