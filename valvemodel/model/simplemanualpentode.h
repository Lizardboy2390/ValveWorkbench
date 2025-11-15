#ifndef SIMPLEMANUALPENTODE_H
#define SIMPLEMANUALPENTODE_H

#include "cohenhelietriode.h"

// SimpleManualPentode: backend-only, web-style pentode shell.
// NOTE: Not yet wired to any UI or fitting path; anodeCurrent uses
// a simplified epk-based formula and will be refined to match
// pentodemodeller.js exactly in a later step.

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
