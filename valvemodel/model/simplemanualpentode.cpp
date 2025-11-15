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

    // 6L6-GC oriented starting values for manual modelling
    if (parameter[PAR_MU])    parameter[PAR_MU]->setValue(9.0);
    if (parameter[PAR_KP])    parameter[PAR_KP]->setValue(350.0);
    if (parameter[PAR_KG1])   parameter[PAR_KG1]->setValue(0.70);
    if (parameter[PAR_KG2])   parameter[PAR_KG2]->setValue(0.18);
    if (parameter[PAR_ALPHA]) parameter[PAR_ALPHA]->setValue(0.30);
    if (parameter[PAR_BETA])  parameter[PAR_BETA]->setValue(0.55);
    if (parameter[PAR_GAMMA]) parameter[PAR_GAMMA]->setValue(1.6);
    if (parameter[PAR_A])     parameter[PAR_A]->setValue(0.05);
}

void SimpleManualPentode::setOptions()
{
    // Do NOT call CohenHelieTriode::setOptions here, because that attempts
    // to set parameter bounds on Ceres parameter blocks that this model
    // does not add (addSample is a no-op for now). Instead, configure only
    // basic solver options so an empty problem is still valid.

    options.max_num_iterations = 1;
    options.max_num_consecutive_invalid_steps = 1;
    options.linear_solver_type = ceres::DENSE_QR;
}

// Gardiner-style anode current so Simple Manual Pentode tracks
// the same behaviour as the main GardinerPentode model.

double SimpleManualPentode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    // Normalize screen voltage to volts for Epk helper (measurement
    // may be kV-like, e.g. 0.250).
    double v2_for_epk = (std::fabs(vg2) < 5.0 ? vg2 * 1000.0 : vg2);
    double epk = cohenHelieEpk(v2_for_epk, vg1);
    // Runtime stability: prevent hard-zero collapse at strong -Vg1
    // during plotting.
    epk = std::max(epk, 1e-6);

    double kg1   = parameter[PAR_KG1]->getValue();
    double kg2   = parameter[PAR_KG2]->getValue();
    double alpha = parameter[PAR_ALPHA]->getValue();
    double beta  = parameter[PAR_BETA]->getValue();
    double gamma = parameter[PAR_GAMMA]->getValue();
    double A     = parameter[PAR_A]->getValue();

    if (kg1 <= 0.0 || kg2 <= 0.0) {
        return 0.0;
    }

    double k     = 1.0 / kg1 - 1.0 / kg2;
    double shift = beta * (1.0 - alpha * vg1);
    double g     = std::exp(-std::pow(shift * va, gamma));
    double scale = 1.0 - g;

    double vco = vg2 / parameter[PAR_LAMBDA]->getValue()
                 - vg1 * parameter[PAR_NU]->getValue()
                 - parameter[PAR_OMEGA]->getValue();
    double psec = parameter[PAR_S]->getValue() * va
                  * (1.0 + std::tanh(-parameter[PAR_AP]->getValue() * (va - vco)));

    double ia = epk * (k * scale + A * va / kg2)
                + parameter[PAR_OS]->getValue() * vg2;

    if (secondaryEmission) {
        ia = ia - epk * psec / kg2;
    }

    if (!std::isfinite(ia) || ia < 0.0) {
        return 0.0;
    }
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
    int i = 0;

    // Core Simple Manual Pentode parameters exposed in a compact set:
    // mu, kp, kg1, kg2, alpha, beta, gamma, a
    updateParameter(labels[i], values[i], parameter[PAR_MU]);    i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]);    i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]);   i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG2]);   i++;
    updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]);  i++;
    updateParameter(labels[i], values[i], parameter[PAR_GAMMA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_A]);     i++;
}

void SimpleManualPentode::updateProperties(QTableWidget *properties)
{
    CohenHelieTriode::updateProperties(properties);
}
