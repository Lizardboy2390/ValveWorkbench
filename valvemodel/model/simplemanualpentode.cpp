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
    // Do NOT call CohenHelieTriode::setOptions here, because that attempts
    // to set parameter bounds on Ceres parameter blocks that this model
    // does not add (addSample is a no-op for now). Instead, configure only
    // basic solver options so an empty problem is still valid.

    options.max_num_iterations = 1;
    options.max_num_consecutive_invalid_steps = 1;
    options.linear_solver_type = ceres::DENSE_QR;
}

// Web-style Simple Manual Pentode anode current.
// epk follows pentodemodeller.js:
//   epk = (vg2 / kp * log(1 + exp(kp * (1/mu - vg1/vg2))))^1.5
// Knee / tail use the same Reefman-style form (alpha, beta, gamma, A).

double SimpleManualPentode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    Q_UNUSED(secondaryEmission);

    double mu    = parameter[PAR_MU]->getValue();
    double kg1   = parameter[PAR_KG1]->getValue();
    double kg2   = parameter[PAR_KG2]->getValue();
    double kp    = parameter[PAR_KP]->getValue();
    double alpha = parameter[PAR_ALPHA]->getValue();
    double beta  = parameter[PAR_BETA]->getValue();
    double gamma = parameter[PAR_GAMMA]->getValue();
    double A     = parameter[PAR_A]->getValue();

    // Basic guards to avoid degenerate or explosive values
    if (vg2 == 0.0 || std::fabs(vg2) < 1e-6) {
        return 0.0;
    }
    if (mu <= 0.0 || kp <= 0.0 || kg1 <= 0.0 || kg2 <= 0.0) {
        return 0.0;
    }

    // Web-style epk: clamp exponent to keep exp() numerically stable
    double inner = kp * (1.0 / mu - vg1 / vg2);
    const double innerClamp = 60.0; // exp(±60) is already extreme
    if (inner > innerClamp)  inner = innerClamp;
    if (inner < -innerClamp) inner = -innerClamp;

    double expInner = std::exp(inner);
    double logTerm  = std::log1p(expInner); // log(1 + exp(inner))
    double base     = (vg2 / kp) * logTerm;
    if (!(std::isfinite(base)) || base <= 0.0) {
        return 0.0;
    }
    double epk = std::pow(base, 1.5);

    double k     = 1.0 / kg1 - 1.0 / kg2;
    double shift = beta * (1.0 - alpha * vg1);
    double g     = 1.0 / (1.0 + std::pow(shift * va, gamma));
    double scale = 1.0 - g;

    double ia    = epk * (k * scale + A * va / kg1);
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
