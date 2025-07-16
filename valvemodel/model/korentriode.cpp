#include "korentriode.h"

struct KorenTriodeResidual {
    KorenTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const kp, const T* const kvb, const T* const x, const T* const mu, T* residual) const {
        T e1t = log(1.0 + exp(kp[0] * (1.0 / mu[0] + vg_ / sqrt(kvb[0] + va_ * va_))));
        T ia = pow((va_ / kp[0]) * e1t, x[0]) / kg[0];
        residual[0] = ia_ - ia;
        return !(isnan(ia) || isinf(ia));
    }

private:
    const double va_;
    const double vg_;
    const double ia_;
};

KorenTriode::KorenTriode()
{

}

void KorenTriode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    // Store the sample for future reference instead of using Ceres
    ValveSample sample;
    sample.va = va;
    sample.ia = ia;
    sample.vg1 = vg1;
    
    // Store in samples vector
    samples.push_back(sample);
    
    // Mark as converged since we're using direct calculation
    converged = true;
}

double KorenTriode::triodeAnodeCurrent(double va, double vg1)
{
    return anodeCurrent(va, vg1);
}

double KorenTriode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    return korenCurrent(va, vg1,
        parameter[PAR_KP]->getValue(),
        parameter[PAR_KVB]->getValue(),
        parameter[PAR_X]->getValue(),
        parameter[PAR_MU]->getValue()) / parameter[PAR_KG1]->getValue();

}

void KorenTriode::fromJson(QJsonObject source)
{
    SimpleTriode::fromJson(source);

    if (source.contains("kp") && source["kp"].isDouble()) {
        parameter[PAR_KP]->setValue(source["kp"].toDouble());
    }

    if (source.contains("kvb") && source["kvb"].isDouble()) {
        parameter[PAR_KVB]->setValue(source["kvb"].toDouble());
    }
}

void KorenTriode::toJson(QJsonObject &model)
{
    SimpleTriode::toJson(model);

    model["kp"] = parameter[PAR_KP]->getValue();
    model["kvb"] = parameter[PAR_KVB]->getValue();

    model["device"] = "triode";
    model["type"] = "koren";
}

void KorenTriode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;
}

QString KorenTriode::getName()
{
    return QString("Koren");
}

int KorenTriode::getType()
{
    return KOREN_TRIODE;
}

void KorenTriode::updateProperties(QTableWidget *properties)
{

}

void KorenTriode::setOptions()
{
    SimpleTriode::setOptions();

    // Set parameter limits for numerical stability
    setLimits(parameter[PAR_KVB], 0.1, 10000.0); // 0.1 <= Kvb <= 10000.0 (avoid zero for sqrt stability)
}

double KorenTriode::korenCurrent(double va, double vg, double kp, double kvb, double a, double mu)
{
    double x1 = std::sqrt(kvb + va * va);
    double x2 = kp * (1 / mu + vg / x1);
    double x3 = std::log(1.0 + std::exp(x2));
    double et = (va / kp) * x3;

    if (et < 0.0) {
        et = 0.0;
    }

    return pow(et, a);
}
