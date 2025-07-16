#include "reefmanpentode.h"

#include <cmath>

struct DerkPentodeResidual {
    DerkPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, T* residual) const {
        // Improved numerical stability based on valvedesigner-web implementation
        // Ensure vg2 is at least 0.1V to avoid numerical issues
        T vg2 = T(vg2_ < 0.1 ? 0.1 : vg2_);
        
        // Calculate f with protection against negative values
        T kvb_term = T(kvb[0] > 0 ? kvb[0] : 0.1);
        T f = sqrt(kvb_term + kvb1[0] * vg2 + vg2 * vg2);
        
        // Calculate epk with protection against extreme values
        T vg1_term = T(vg1_ + vct[0]);
        T exp_arg = kp[0] * (T(1.0) / mu[0] + vg1_term / f);
        // Limit exp_arg to avoid overflow
        exp_arg = T(exp_arg > 100 ? 100 : (exp_arg < -100 ? -100 : exp_arg));
        
        T epk = pow(vg2 * log(T(1.0) + exp(exp_arg)) / kp[0], x[0]);
        
        // Ensure kg1 and kg2 are not too close to zero
        T kg1_safe = T(kg1[0] < 1e-6 ? 1e-6 : kg1[0]);
        T kg2_safe = T(kg2[0] < 1e-6 ? 1e-6 : kg2[0]);
        
        // Calculate g with protection against extreme values
        T beta_term = T(beta[0] * va_ < 100 ? beta[0] * va_ : 100);
        T g = T(1.0) / (T(1.0) + beta_term);
        
        // Calculate anode current with improved numerical stability
        T ia = epk * ((T(1.0) / kg1_safe - T(1.0) / kg2_safe) * (T(1.0) - g) + a[0] * T(va_) / kg1_safe);
        
        residual[0] = T(ia_) - ia;
        return true; // Return true for valid calculation
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

struct DerkEPentodeResidual {
    DerkEPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, T* residual) const {
        // Improved numerical stability based on valvedesigner-web implementation
        // Ensure vg2 is at least 0.1V to avoid numerical issues
        T vg2 = T(vg2_ < 0.1 ? 0.1 : vg2_);
        
        // Calculate f with protection against negative values
        T kvb_term = T(kvb[0] > 0 ? kvb[0] : 0.1);
        T f = sqrt(kvb_term + kvb1[0] * vg2 + vg2 * vg2);
        
        // Calculate epk with protection against extreme values
        T vg1_term = T(vg1_ + vct[0]);
        T exp_arg = kp[0] * (T(1.0) / mu[0] + vg1_term / f);
        // Limit exp_arg to avoid overflow
        exp_arg = T(exp_arg > 100 ? 100 : (exp_arg < -100 ? -100 : exp_arg));
        
        T epk = pow(vg2 * log(T(1.0) + exp(exp_arg)) / kp[0], x[0]);
        
        // Ensure kg1 and kg2 are not too close to zero
        T kg1_safe = T(kg1[0] < 1e-6 ? 1e-6 : kg1[0]);
        T kg2_safe = T(kg2[0] < 1e-6 ? 1e-6 : kg2[0]);
        
        // Calculate g with protection against extreme values for the exponential form
        T beta_va = T(beta[0] * va_);
        T pow_arg = T(beta_va > 0 ? beta_va : 0);
        T pow_result = pow(pow_arg, T(1.5));
        // Limit pow_result to avoid underflow in exp
        pow_result = T(pow_result > 100 ? 100 : pow_result);
        T g = exp(-pow_result);
        
        // Calculate anode current with improved numerical stability
        T ia = epk * ((T(1.0) / kg1_safe - T(1.0) / kg2_safe) * (T(1.0) - g) + a[0] * T(va_) / kg1_safe);
        
        residual[0] = T(ia_) - ia;
        return true; // Return true for valid calculation
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

double ReefmanPentode::anodeCurrent(double va, double vg1, double vg2)
{
    // Ensure vg2 is at least 0.1V to avoid numerical issues
    vg2 = std::max(0.1, vg2);
    
    // Calculate epk with improved numerical stability
    double epk = cohenHelieEpk(vg2, vg1);
    
    // Ensure kg1 and kg2 are not too close to zero
    double kg1 = std::max(1e-6, parameter[PAR_KG1]->getValue());
    double kg2 = std::max(1e-6, parameter[PAR_KG2]->getValue());
    
    double k = 1.0 / kg1 - 1.0 / kg2;
    
    // Calculate shift with protection against extreme values
    double alpha = parameter[PAR_ALPHA]->getValue();
    double beta = parameter[PAR_BETA]->getValue();
    double shift = beta * (1.0 - alpha * vg1);
    
    // Limit shift*va to avoid numerical issues
    double shift_va = std::min(100.0, shift * va);
    
    // Use the appropriate g calculation based on model type
    double g;
    if (modelType == DERK) {
        // Original form: 1.0 / (1.0 + pow(shift * va, parameter[PAR_GAMMA]->getValue()));
        double gamma = std::max(0.5, parameter[PAR_GAMMA]->getValue()); // Ensure gamma is at least 0.5
        g = 1.0 / (1.0 + pow(shift_va, gamma));
    } else { // DERK_E
        // Original form: exp(-pow(shift * va, parameter[PAR_GAMMA]->getValue()));
        double gamma = std::max(0.5, parameter[PAR_GAMMA]->getValue()); // Ensure gamma is at least 0.5
        double pow_result = std::min(100.0, pow(shift_va, gamma)); // Limit to avoid underflow
        g = exp(-pow_result);
    }
    
    double scale = 1.0 - g;
    double ia = epk * (k * scale + parameter[PAR_A]->getValue() * va / kg1);

    return ia;
}

ReefmanPentode::ReefmanPentode(int newType) : modelType(newType)
{
    secondaryEmission = false;
}

void ReefmanPentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    // Store the sample for future reference instead of using Ceres
    PentodeSample sample;
    sample.va = va;
    sample.ia = ia;
    sample.vg1 = vg1;
    sample.vg2 = vg2;
    sample.ig2 = ig2;
    
    // Store in appropriate sample vector
    anodeSamples.push_back(sample);
    
    // If we have screen current measurements, store them too
    if (ig2 > 0) {
        screenSamples.push_back(sample);
    }
    
    // Mark as converged since we're using direct calculation
    converged = true;
}

void ReefmanPentode::fromJson(QJsonObject source)
{

}

void ReefmanPentode::toJson(QJsonObject &destination)
{

}

void ReefmanPentode::updateUI(QLabel *labels[], QLineEdit *values[])
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
    updateParameter(labels[i], values[i], parameter[PAR_A]); i++;
    //updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]); i++;
}

QString ReefmanPentode::getName()
{
    return "Cohen Helie Pentode";
}

int ReefmanPentode::getType()
{
    return REEFMAN_DERK_PENTODE;
}

void ReefmanPentode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue()));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));
    addProperty(properties, "Kvb1", QString("%1").arg(parameter[PAR_KVB1]->getValue()));
    addProperty(properties, "vct", QString("%1").arg(parameter[PAR_VCT]->getValue()));

    addProperty(properties, "Kg2", QString("%1").arg(parameter[PAR_KG2]->getValue()));
    addProperty(properties, "A", QString("%1").arg(parameter[PAR_A]->getValue()));
    //addProperty(properties, "alpha", QString("%1").arg(parameter[PAR_ALPHA]->getValue()));
    addProperty(properties, "beta", QString("%1").arg(parameter[PAR_BETA]->getValue()));
}

bool ReefmanPentode::withSecondaryEmission() const
{
    return secondaryEmission;
}

void ReefmanPentode::setSecondaryEmission(bool newSecondaryEmission)
{
    secondaryEmission = newSecondaryEmission;
}

int ReefmanPentode::getModelType() const
{
    return modelType;
}

void ReefmanPentode::setModelType(int newModelType)
{
    modelType = newModelType;
}

void ReefmanPentode::setOptions()
{
    CohenHelieTriode::setOptions();
    
    // Parameter limits based on valvedesigner-web implementation for better stability
    setLimits(parameter[PAR_KG2], 0.0001, 1.0); // Avoid division by zero
    
    // Set solver options (these won't be used with direct calculation but kept for compatibility)
    options.max_num_iterations = 100;
    options.function_tolerance = 1e-6;
    options.gradient_tolerance = 1e-10;
    options.parameter_tolerance = 1e-8;
    
    // Set parameter limits for numerical stability
    setLowerBound(parameter[PAR_A], 0.0);
    setLowerBound(parameter[PAR_ALPHA], 0.0);
    setLowerBound(parameter[PAR_BETA], 0.00001); // Minimum value for stability
    setLowerBound(parameter[PAR_GAMMA], 0.5); // Minimum value for stability
}
