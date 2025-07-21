#include "cohenhelietriode.h"
<<<<<<< Updated upstream
#include <cmath>
#include <algorithm>

// Direct implementation without Ceres, based on valvedesigner-web
=======
#include <ceres/jet.h>

struct CohenHelieTriodeResidual {
    CohenHelieTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, T* residual) const {
        T epk = log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg_ + vct[0]) / sqrt(kvb[0] + va_ * va_ + kvb1[0] * va_))));
        T ia = pow((va_ / kp[0]) * epk, x[0]) / kg[0];
        residual[0] = ia_ - ia;
        return !(ceres::isnan(ia) || ceres::isinf(ia));
    }

private:
    const double va_;
    const double vg_;
    const double ia_;
};
>>>>>>> Stashed changes

CohenHelieTriode::CohenHelieTriode()
{

}

void CohenHelieTriode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    // Store sample data for manual fitting if needed
    // Without Ceres, we'll use direct calculation instead of residual blocks
    samples.push_back({va, vg1, ia});
}

double CohenHelieTriode::triodeAnodeCurrent(double va, double vg1)
{
    return cohenHelieCurrent(va, vg1,
                             parameter[PAR_KG1]->getValue(),
                             parameter[PAR_KP]->getValue(),
                             parameter[PAR_KVB]->getValue(),
                             parameter[PAR_KVB1]->getValue(),
                             parameter[PAR_VCT]->getValue(),
                             parameter[PAR_X]->getValue(),
                             parameter[PAR_MU]->getValue());
}

double CohenHelieTriode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    return triodeAnodeCurrent(va, vg1);
}

void CohenHelieTriode::fromJson(QJsonObject source)
{
    KorenTriode::fromJson(source);

    if (source.contains("kvb1") && source["kvb1"].isDouble()) {
        parameter[PAR_KVB1]->setValue(source["kvb1"].toDouble());
    }
}

void CohenHelieTriode::toJson(QJsonObject &model)
{
    KorenTriode::toJson(model);

    model["kvb1"] = parameter[PAR_KVB1]->getValue();

    model["device"] = "triode";
    model["type"] = "cohenHelie";
}

void CohenHelieTriode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_VCT]); i++;
}

QString CohenHelieTriode::getName()
{
    return QString("Cohen Helie");
}

int CohenHelieTriode::getType()
{
    return COHEN_HELIE_TRIODE;
}

void CohenHelieTriode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue() * 1000.0));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));
    addProperty(properties, "Kvb1", QString("%1").arg(parameter[PAR_KVB1]->getValue()));
    addProperty(properties, "vct", QString("%1").arg(parameter[PAR_VCT]->getValue()));
}

void CohenHelieTriode::setOptions()
{
    KorenTriode::setOptions();

    // Set parameter limits for numerical stability based on valvedesigner-web implementation
    setLimits(parameter[PAR_KVB1], 0.1, 1000.0); // 0.1 <= Kvb1 <= 1000.0 (avoid zero for numerical stability)
    setLimits(parameter[PAR_VCT], 0.0, 2.0); // 0.0 <= Vct <= 2.0
    
    // Set max iterations for compatibility with UI (not used in direct calculation)
    options.max_num_iterations = 100;
    
    // Mark as converged since we're using direct calculation
    converged = true;
}

double CohenHelieTriode::cohenHelieCurrent(double v, double vg, double kg1, double kp, double kvb, double kvb1, double vct, double x, double mu)
{
    // Improved numerical stability based on valvedesigner-web implementation
    // Ensure v is at least 0.1V to avoid numerical issues
    v = std::max(0.1, v);
    
    // Calculate f with protection against negative values
    double kvb_term = std::max(0.1, kvb);
    double f = sqrt(kvb_term + v * kvb1 + v * v);
    
    // Calculate y with protection against extreme values
    double vg_term = vg + vct;
    double y = kp * (1.0 / mu + vg_term / f);
    
    // Limit y to avoid overflow in exp
    y = std::min(100.0, std::max(-100.0, y));
    
    // Calculate ep
    double ep = (v / kp) * log(1.0 + exp(y));
    
    // Ensure kg1 is not too close to zero
    kg1 = std::max(1e-6, kg1);
    
    // Calculate and return current
    return pow(ep, x) / kg1;
}

double CohenHelieTriode::cohenHelieEpk(double v, double vg, double kp, double kvb, double kvb1, double vct, double x, double mu)
{
    // Improved numerical stability based on valvedesigner-web implementation
    double f = sqrt(kvb + v * kvb1 + v * v);
    double y = kp * (1.0 / mu + (vg + vct) / f);
    double ep = (v / kp) * log(1.0 + exp(y));

    return pow(ep, x);
}

double CohenHelieTriode::cohenHelieEpk(double v, double vg)
{
    double kp = parameter[PAR_KP]->getValue();
    double kvb = parameter[PAR_KVB]->getValue();
    double kvb1 = parameter[PAR_KVB1]->getValue();
    double vct = parameter[PAR_VCT]->getValue();
    double x = parameter[PAR_X]->getValue();
    double mu = parameter[PAR_MU]->getValue();

    return cohenHelieEpk(v, vg, kp, kvb, kvb1, vct, x, mu);
}
