#include "simplemanualpentode.h"

#include <cmath>

#include "ceres/ceres.h"

// Backend-only SimpleManualPentode. For now this uses a simplified
// epk-based anode current similar to Reefman/Gardiner. In a later
// step it will be updated to exactly match the web pentodemodeller.js
// equations once fully ported.

SimpleManualPentode::SimpleManualPentode()
{
    setOptions();
}

void SimpleManualPentode::setOptions()
{
    // Reuse CohenHelieTriode options for now (bounds, solver options).
    CohenHelieTriode::setOptions();
}

// Simplified pentode anode current using Cohen-Helie epk helper.
// This is intentionally conservative and will be refined later.

double SimpleManualPentode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    Q_UNUSED(secondaryEmission);

    // Use Cohen-Helie epk helper from base class; vg2, vg1 in volts.
    double epk = cohenHelieEpk(vg2, vg1);

    double kg1 = parameter[PAR_KG1]->getValue();
    double kg2 = parameter[PAR_KG2]->getValue();
    double alpha = parameter[PAR_ALPHA]->getValue();
    double beta  = parameter[PAR_BETA]->getValue();
    double gamma = parameter[PAR_GAMMA]->getValue();
    double A     = parameter[PAR_A]->getValue();

    if (kg1 <= 0.0 || kg2 <= 0.0) {
        return 0.0;
    }

    double k     = 1.0 / kg1 - 1.0 / kg2;
    double shift = beta * (1.0 - alpha * vg1);
    double g     = 1.0 / (1.0 + std::pow(shift * va, gamma));
    double scale = 1.0 - g;

    double ia    = epk * (k * scale + A * va / kg1);
    return ia;
}

// For now, SimpleManualPentode does not add any Ceres residuals; it is
// intended to be used in a manual/slider-driven mode. addSample is
// implemented as a no-op so that fitting paths can safely call it
// without affecting the solver state.

void SimpleManualPentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    Q_UNUSED(va);
    Q_UNUSED(ia);
    Q_UNUSED(vg1);
    Q_UNUSED(vg2);
    Q_UNUSED(ig2);
}

void SimpleManualPentode::fromJson(QJsonObject source)
{
    CohenHelieTriode::fromJson(source);

    if (source.contains("kg2") && source["kg2"].isDouble()) {
        parameter[PAR_KG2]->setValue(source["kg2"].toDouble());
    }
    if (source.contains("alpha") && source["alpha"].isDouble()) {
        parameter[PAR_ALPHA]->setValue(source["alpha"].toDouble());
    }
    if (source.contains("beta") && source["beta"].isDouble()) {
        parameter[PAR_BETA]->setValue(source["beta"].toDouble());
    }
    if (source.contains("gamma") && source["gamma"].isDouble()) {
        parameter[PAR_GAMMA]->setValue(source["gamma"].toDouble());
    }
    if (source.contains("a") && source["a"].isDouble()) {
        parameter[PAR_A]->setValue(source["a"].toDouble());
    }
}

void SimpleManualPentode::toJson(QJsonObject &destination)
{
    CohenHelieTriode::toJson(destination);
    destination["kg2"]   = parameter[PAR_KG2]->getValue();
    destination["alpha"] = parameter[PAR_ALPHA]->getValue();
    destination["beta"]  = parameter[PAR_BETA]->getValue();
    destination["gamma"] = parameter[PAR_GAMMA]->getValue();
    destination["a"]     = parameter[PAR_A]->getValue();
}

QString SimpleManualPentode::getName()
{
    return "Simple Manual Pentode";
}

int SimpleManualPentode::getType()
{
    return GARDINER_PENTODE; // placeholder; not used heavily in current code
}

void SimpleManualPentode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    // Reuse CohenHelieTriode UI mapping for now.
    CohenHelieTriode::updateUI(labels, values);
}

void SimpleManualPentode::updateProperties(QTableWidget *properties)
{
    CohenHelieTriode::updateProperties(properties);
}
