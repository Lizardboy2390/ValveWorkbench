#include "gardinerpentode.h"
#include <ceres/jet.h>

//#include <cmath>

struct UnifiedPentodeIaResidual {
    UnifiedPentodeIaResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, const T* const gamma, const T* const os, T* residual) const {
        // Improved numerical stability based on valvedesigner-web implementation
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        // Calculate epk using the Cohen-Helie model with improved stability
        T y = kp[0] * (T(1.0) / mu[0] + (vg1_ + vct[0]) / f);
        T ep = (vg2_ / kp[0]) * log(T(1.0) + exp(y));
        T epk = pow(ep, x[0]);
        
        // Calculate current distribution between anode and screen
        T shift = beta[0] * (T(1.0) - alpha[0] * vg1_);
        T g = exp(-pow(shift * va_, gamma[0]));
<<<<<<< Updated upstream
        
        // Handle potential numerical instability
        if (isnan(g) || isinf(g)) {
            g = T(1.0); // More stable default value
=======
        //T g = 1.0 / (1.0 + pow(shift * va_, gamma[0]));
        if (ceres::isnan(g)) { // Should only happen if Va is 0 and this is a better test than == 0.0
            g = mu[0] / mu[0];
>>>>>>> Stashed changes
        }
        
        T scale = T(1.0) - g;
        T k = T(1.0) / kg1[0] - T(1.0) / kg2[0];
        T ia = epk * (k * scale + a[0] * va_ / kg2[0]) + os[0] * vg2_;

<<<<<<< Updated upstream
        if (!(isnan(ia) || isinf(ia))) {
            residual[0] = ia_ - ia;
=======
        //double w = exp(va_/ 250.0);
        if (!(ceres::isnan(ia) || ceres::isinf(ia))) {
            //residual[0] = (ia_ - ia) * w;
            residual[0] = (ia_ - ia);
>>>>>>> Stashed changes
        } else {
            return false;
        }
        return true;
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

struct UnifiedPentodeIaSEResidual {
    UnifiedPentodeIaSEResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, const T* const gamma, const T* const os, const T* const omega, const T* const lambda, const T* const nu, const T* const s, const T* const ap, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T shift = beta[0] * (1.0 - alpha[0] * vg1_);
        T g = exp(-pow(shift * va_, gamma[0]));
        //T g = 1.0 / (1.0 + pow(shift * va_, gamma[0]));
        if (ceres::isnan(g)) { // Should only happen if Va is 0 and this is a better test than == 0.0
            g = mu[0] / mu[0];
        }
        T scale = 1.0 - g;
        T vco = vg2_ / lambda[0] - vg1_ * nu[0] - omega[0];
        T psec = s[0] * va_ * (1.0 + tanh(-ap[0] * (va_ - vco)));
        T ia = epk * ((1.0 / kg1[0] - 1.0 / kg2[0]) * scale + a[0] * va_ / kg2[0] - psec / kg2[0]) + os[0] * vg2_;

        //double w = exp(va_/ 250.0);
        if (!(ceres::isnan(ia) || ceres::isinf(ia))) {
            //residual[0] = (ia_ - ia) * w;
            residual[0] = (ia_ - ia);
        } else {
            return false;
        }
        return true;
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

struct UnifiedPentodeIg2Residual {
    UnifiedPentodeIg2Residual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg3, const T* const a, const T* const tau, const T* const rho, const T* const theta, const T* const psi, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T shift = rho[0] * (1.0 - tau[0] * vg1_);
        T h = exp(-pow(shift * va_, theta[0]));
        //T h = 1.0 / (1.0 + pow(shift * va_, theta[0]));
        if (ceres::isnan(h)) { // Should only happen if Va is 0 and this is a better test than == 0.0
            h = mu[0] / mu[0];
        }
        T ig2 = epk * (1.0 + psi[0] * h) / kg3[0] - epk * a[0] * va_ / kg3[0];
        //T ig2 = epk * (1.0 + psi[0] * h) / kg3[0];

        //double w = exp(va_/ 250.0);
        if (!(ceres::isnan(ig2) || ceres::isinf(ig2))) {
            //residual[0] = (ig2_ - ig2) * w;
            residual[0] = (ig2_ - ig2);
        } else {
            return false;
        }
        return true;
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

struct UnifiedPentodeIg2SEResidual {
    UnifiedPentodeIg2SEResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg3, const T* const a, const T* const tau, const T* const rho, const T* const theta, const T* const psi, const T* const omega, const T* const lambda, const T* const nu, const T* const s,
                    const T* const ap, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T shift = rho[0] * (1.0 - tau[0] * vg1_);
        T h = exp(-pow(shift * va_, theta[0]));
        //T h = 1.0 / (1.0 + pow(shift * va_, theta[0]));
        if (ceres::isnan(h)) { // Should only happen if Va is 0 and this is a better test than == 0.0
            h = mu[0] / mu[0];
        }
        T vco = vg2_ / lambda[0] - vg1_ * nu[0] - omega[0];
        T psec = s[0] * va_ * (1.0 + tanh(-ap[0] * (va_ - vco)));

        T ig2 = epk * (1.0 + psi[0] * h + psec) / kg3[0] - epk * a[0] * va_ / kg3[0];
        //T ig2 = epk * (1.0 + psi[0] * h + psec) / kg3[0];

        //double w = exp(va_/ 250.0);
        if (!(ceres::isnan(ig2) || ceres::isinf(ig2))) {
            //residual[0] = (ig2_ - ig2) * w;
            residual[0] = (ig2_ - ig2);
        } else {
            return false;
        }
        return true;
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

double GardinerPentode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    // Improved implementation based on valvedesigner-web
    double epk = cohenHelieEpk(vg2, vg1);
    double k = 1.0 / parameter[PAR_KG1]->getValue() - 1.0 / parameter[PAR_KG2]->getValue();
    double shift = parameter[PAR_BETA]->getValue() * (1.0 - parameter[PAR_ALPHA]->getValue() * vg1);
    
    // Use more stable exponential function with proper error handling
    double g;
    double gamma = parameter[PAR_GAMMA]->getValue();
    double shiftVa = shift * va;
    
    if (va <= 0 || std::isnan(shiftVa) || std::isinf(shiftVa)) {
        g = 1.0; // Handle edge case
    } else {
        g = std::exp(-std::pow(shiftVa, gamma));
    }
    
    double scale = 1.0 - g;
    double ia = epk * (k * scale + parameter[PAR_A]->getValue() * va / parameter[PAR_KG2]->getValue()) + parameter[PAR_OS]->getValue() * vg2;
    
    if (secondaryEmission) {
        double vco = vg2 / parameter[PAR_LAMBDA]->getValue() - vg1 * parameter[PAR_NU]->getValue() - parameter[PAR_OMEGA]->getValue();
        double psec = parameter[PAR_S]->getValue() * va * (1.0 + std::tanh(-parameter[PAR_AP]->getValue() * (va - vco)));
        ia = ia - epk * psec / parameter[PAR_KG2]->getValue();
    }
    
    return std::max(ia, 0.0); // Ensure current is never negative
}

double GardinerPentode::screenCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    // Improved implementation based on valvedesigner-web
    double epk = cohenHelieEpk(vg2, vg1);
    double shift = parameter[PAR_RHO]->getValue() * (1.0 - parameter[PAR_TAU]->getValue() * vg1);
    
    // Use more stable exponential function with proper error handling
    double h;
    double theta = parameter[PAR_THETA]->getValue() * 0.9; // 0.9 factor for stability
    double shiftVa = shift * va;
    
    if (va <= 0 || std::isnan(shiftVa) || std::isinf(shiftVa)) {
        h = 1.0; // Handle edge case
    } else {
        h = std::exp(-std::pow(shiftVa, theta));
    }
    
    double ig2 = epk * (1.0 + parameter[PAR_PSI]->getValue() * h) / parameter[PAR_KG2A]->getValue() - 
                 epk * parameter[PAR_A]->getValue() * va / parameter[PAR_KG2A]->getValue();
    
    if (secondaryEmission) {
        double vco = vg2 / parameter[PAR_LAMBDA]->getValue() - vg1 * parameter[PAR_NU]->getValue() - parameter[PAR_OMEGA]->getValue();
        double psec = parameter[PAR_S]->getValue() * va * (1.0 + std::tanh(-parameter[PAR_AP]->getValue() * (va - vco)));
        ig2 = ig2 + epk * psec / parameter[PAR_KG2A]->getValue();
    }
    
    return std::max(ig2, 0.0); // Ensure current is never negative
}

GardinerPentode::GardinerPentode()
{
    secondaryEmission = false;
}

void GardinerPentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    // Store sample data for direct calculation approach without Ceres
    PentodeSample sample;
    sample.va = va;
    sample.vg1 = vg1;
    sample.ia = ia;
    sample.vg2 = vg2;
    sample.ig2 = ig2;
    
    // Store the sample based on the current mode
    if (mode == NORMAL_MODE) {
        anodeSamples.push_back(sample);
        screenSamples.push_back(sample);
    } else if (mode == SCREEN_MODE) {
        // Copy parameter values for screen current calculation
        parameter[PAR_TAU]->setValue(parameter[PAR_ALPHA]->getValue());
        parameter[PAR_RHO]->setValue(parameter[PAR_BETA]->getValue());
        parameter[PAR_THETA]->setValue(parameter[PAR_GAMMA]->getValue());
        parameter[PAR_KG2A]->setValue(parameter[PAR_KG2]->getValue());
        
        screenSamples.push_back(sample);
    } else if (mode == ANODE_REMODEL_MODE) {
        anodeRemodelSamples.push_back(sample);
    }
    
    // Mark as converged since we're using direct calculation
    converged = true;
}

void GardinerPentode::fromJson(QJsonObject source)
{
    CohenHelieTriode::fromJson(source);
    
    if (source.contains("kg2") && source["kg2"].isDouble()) {
        parameter[PAR_KG2]->setValue(source["kg2"].toDouble() / 1000.0);
    }
    
    if (source.contains("a") && source["a"].isDouble()) {
        parameter[PAR_A]->setValue(source["a"].toDouble());
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
    
    if (source.contains("kg2a") && source["kg2a"].isDouble()) {
        parameter[PAR_KG2A]->setValue(source["kg2a"].toDouble() / 1000.0);
    }
    
    if (source.contains("tau") && source["tau"].isDouble()) {
        parameter[PAR_TAU]->setValue(source["tau"].toDouble());
    }
    
    if (source.contains("rho") && source["rho"].isDouble()) {
        parameter[PAR_RHO]->setValue(source["rho"].toDouble());
    }
    
    if (source.contains("theta") && source["theta"].isDouble()) {
        parameter[PAR_THETA]->setValue(source["theta"].toDouble());
    }
    
    if (source.contains("psi") && source["psi"].isDouble()) {
        parameter[PAR_PSI]->setValue(source["psi"].toDouble());
    }
    
    if (source.contains("omega") && source["omega"].isDouble()) {
        parameter[PAR_OMEGA]->setValue(source["omega"].toDouble());
    }
    
    if (source.contains("lambda") && source["lambda"].isDouble()) {
        parameter[PAR_LAMBDA]->setValue(source["lambda"].toDouble());
    }
    
    if (source.contains("nu") && source["nu"].isDouble()) {
        parameter[PAR_NU]->setValue(source["nu"].toDouble());
    }
    
    if (source.contains("s") && source["s"].isDouble()) {
        parameter[PAR_S]->setValue(source["s"].toDouble());
    }

    if (source.contains("ap") && source["ap"].isDouble()) {
        parameter[PAR_AP]->setValue(source["ap"].toDouble());
    }

    if (source.contains("os") && source["os"].isDouble()) {
        parameter[PAR_OS]->setValue(source["os"].toDouble());
    }
}

void GardinerPentode::toJson(QJsonObject &model)
{
    CohenHelieTriode::toJson(model);

    model["kg2"] = parameter[PAR_KG2]->getValue() * 1000.0;
    model["a"] = parameter[PAR_A]->getValue();
    model["alpha"] = parameter[PAR_ALPHA]->getValue();
    model["beta"] = parameter[PAR_BETA]->getValue();
    model["gamma"] = parameter[PAR_GAMMA]->getValue();

    model["kg2a"] = parameter[PAR_KG2A]->getValue() * 1000.0;
    model["tau"] = parameter[PAR_TAU]->getValue();
    model["rho"] = parameter[PAR_RHO]->getValue();
    model["theta"] = parameter[PAR_THETA]->getValue();
    model["psi"] = parameter[PAR_PSI]->getValue();

    model["omega"] = parameter[PAR_OMEGA]->getValue();
    model["lambda"] = parameter[PAR_LAMBDA]->getValue();
    model["nu"] = parameter[PAR_NU]->getValue();
    model["s"] = parameter[PAR_S]->getValue();
    model["ap"] = parameter[PAR_AP]->getValue();

    model["os"] = parameter[PAR_OS]->getValue();

    model["device"] = "pentode";
    model["type"] = "gardiner";
}

void GardinerPentode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_VCT]); i++;

    updateParameter(labels[i], values[i], parameter[PAR_KG2]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG2A]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_A]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_GAMMA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_OS]); i++;

    updateParameter(labels[i], values[i], parameter[PAR_TAU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_RHO]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_THETA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_PSI]); i++;

    if (preferences->useSecondaryEmission()) {
        updateParameter(labels[i], values[i], parameter[PAR_OMEGA]); i++;
        updateParameter(labels[i], values[i], parameter[PAR_LAMBDA]); i++;
        updateParameter(labels[i], values[i], parameter[PAR_NU]); i++;
        updateParameter(labels[i], values[i], parameter[PAR_S]); i++;
        updateParameter(labels[i], values[i], parameter[PAR_AP]); i++;
    }
}

QString GardinerPentode::getName()
{
    return "Cohen Helie Pentode";
}

int GardinerPentode::getType()
{
    return GARDINER_PENTODE;
}

void GardinerPentode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue() * 1000.0));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));
    addProperty(properties, "Kvb1", QString("%1").arg(parameter[PAR_KVB1]->getValue()));
    addProperty(properties, "vct", QString("%1").arg(parameter[PAR_VCT]->getValue()));

    addProperty(properties, "Kg2", QString("%1").arg(parameter[PAR_KG2]->getValue() * 1000.0));
    addProperty(properties, "A", QString("%1").arg(parameter[PAR_A]->getValue()));
    addProperty(properties, "alpha", QString("%1").arg(parameter[PAR_ALPHA]->getValue()));
    addProperty(properties, "beta", QString("%1").arg(parameter[PAR_BETA]->getValue()));
    addProperty(properties, "gamma", QString("%1").arg(parameter[PAR_GAMMA]->getValue()));

    addProperty(properties, "Kg2a", QString("%1").arg(parameter[PAR_KG2A]->getValue() * 1000.0));
    addProperty(properties, "tau", QString("%1").arg(parameter[PAR_TAU]->getValue()));
    addProperty(properties, "rho", QString("%1").arg(parameter[PAR_RHO]->getValue()));
    addProperty(properties, "theta", QString("%1").arg(parameter[PAR_THETA]->getValue()));
    addProperty(properties, "psi", QString("%1").arg(parameter[PAR_PSI]->getValue()));

    if (preferences->useSecondaryEmission()) {
        addProperty(properties, "omega", QString("%1").arg(parameter[PAR_OMEGA]->getValue()));
        addProperty(properties, "lambda", QString("%1").arg(parameter[PAR_LAMBDA]->getValue()));
        addProperty(properties, "nu", QString("%1").arg(parameter[PAR_NU]->getValue()));
        addProperty(properties, "S", QString("%1").arg(parameter[PAR_S]->getValue()));
        addProperty(properties, "Ap", QString("%1").arg(parameter[PAR_AP]->getValue()));
    }

    addProperty(properties, "os", QString("%1").arg(parameter[PAR_OS]->getValue()));
}

void GardinerPentode::setOptions()
{
    CohenHelieTriode::setOptions();

    // Parameter limits based on valvedesigner-web implementation for better stability
    setLimits(parameter[PAR_KG2], 0.0001, 1.0); // Avoid division by zero
    setLimits(parameter[PAR_A], 0.0, 1.0e-3); // 0.0 <= A <= 0.001
    setLimits(parameter[PAR_ALPHA], 0.0, 10.0); // 0.0 <= Alpha <= 10.0
    setLimits(parameter[PAR_BETA], 0.0, 10.0); // 0.0 <= Beta <= 10.0
    setLimits(parameter[PAR_GAMMA], 0.5, 10.0); // 0.5 <= Gamma <= 10.0 (avoid near-zero values)

    setLimits(parameter[PAR_KG2A], 0.0001, 1.0); // Avoid division by zero
    setLimits(parameter[PAR_TAU], 0.0, 10.0); // 0.0 <= Tau <= 10.0
    setLimits(parameter[PAR_RHO], 0.0, 10.0); // 0.0 <= Rho <= 10.0
    setLimits(parameter[PAR_THETA], 0.5, 10.0); // 0.5 <= Theta <= 10.0 (avoid near-zero values)
    setLimits(parameter[PAR_PSI], 0.0, 10.0); // 0.0 <= Psi <= 10.0

    setLimits(parameter[PAR_OMEGA], 0.0, 100.0); // 0.0 <= Omega <= 100.0
    setLimits(parameter[PAR_LAMBDA], 0.1, 10.0); // 0.1 <= Lambda <= 10.0
    setLimits(parameter[PAR_NU], 0.0, 10.0); // 0.0 <= Nu <= 10.0
    setLimits(parameter[PAR_S], 0.0, 1.0e-3); // 0.0 <= S <= 0.001
    setLimits(parameter[PAR_AP], 0.01, 1.0); // 0.01 <= Ap <= 1.0 (avoid near-zero values)
    setLimits(parameter[PAR_OS], 0.0, 1.0e-3); // 0.0 <= Os <= 0.001

    // Set max iterations for compatibility with UI (not used in direct calculation)
    options.max_num_iterations = 100;
    screenOptions.max_num_iterations = 100;
    // Removed anodeRemodelOptions reference as it's no longer needed with direct calculation
    
    // Mark as converged since we're using direct calculation
    converged = true;
    
    // Copy parameter values for screen current calculation when in screen mode
    if (mode == SCREEN_MODE) {
        parameter[PAR_TAU]->setValue(parameter[PAR_ALPHA]->getValue());
        parameter[PAR_RHO]->setValue(parameter[PAR_BETA]->getValue());
        parameter[PAR_THETA]->setValue(parameter[PAR_GAMMA]->getValue());
        parameter[PAR_KG2A]->setValue(parameter[PAR_KG2]->getValue());
    }
}
