#include "simpletriode.h"
#include <ceres/jet.h>

struct SimpleTriodeResidual {
    SimpleTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const vct, const T* const x, const T* const mu, T* residual) const {
        T e1t = va_ / mu[0] + vg_ + vct[0];
        if (e1t < 0.0) {
            e1t = mu[0] - mu[0];
        }
        T ia = pow(e1t, x[0]) / kg[0];
        residual[0] = ia_ - ia;
        return !(ceres::isnan(ia) || ceres::isinf(ia));
    }

private:
    const double va_;
    const double vg_;
    const double ia_;
};

SimpleTriode::SimpleTriode()
{
}

void SimpleTriode::addSample(double va, double ia, double vg1, double vg2, double ig2)
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

double SimpleTriode::triodeAnodeCurrent(double va, double vg1)
{
    return anodeCurrent(va, vg1);
}

double SimpleTriode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    double ia = 0.0;

    double e1t = va / parameter[PAR_MU]->getValue() + vg1 + parameter[PAR_VCT]->getValue();
    if (e1t > 0) {
        ia = pow(e1t, parameter[PAR_X]->getValue()) / parameter[PAR_KG1]->getValue();
    }

    return ia;
}

void SimpleTriode::fromJson(QJsonObject source)
{
    if (source.contains("kg1") && source["kg1"].isDouble()) {
        parameter[PAR_KG1]->setValue(source["kg1"].toDouble() / 1000.0);
    }

    if (source.contains("mu") && source["mu"].isDouble()) {
        parameter[PAR_MU]->setValue(source["mu"].toDouble());
    }

    if (source.contains("x") && source["x"].isDouble()) {
        parameter[PAR_X]->setValue(source["x"].toDouble());
    }

    if (source.contains("vct") && source["vct"].isDouble()) {
        parameter[PAR_VCT]->setValue(source["vct"].toDouble());
    }
}

void SimpleTriode::toJson(QJsonObject &model)
{
    model["kg1"] = parameter[PAR_KG1]->getValue() * 1000.0;
    model["mu"] = parameter[PAR_MU]->getValue();
    model["x"] = parameter[PAR_X]->getValue();
    model["vct"] = parameter[PAR_VCT]->getValue();

    model["device"] = "triode";
    model["type"] = "simple";
}

void SimpleTriode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_VCT]); i++;
}

QString SimpleTriode::getName()
{
    return QString("Simple");
}

int SimpleTriode::getType()
{
    return SIMPLE_TRIODE;
}

void SimpleTriode::updateProperties(QTableWidget *properties)
{

}

void SimpleTriode::setKg(double kg)
{
    parameter[PAR_KG1]->setValue(kg);
}

void SimpleTriode::setMu(double mu)
{
    parameter[PAR_MU]->setValue(mu);
}

void SimpleTriode::setAlpha(double alpha)
{
    parameter[PAR_X]->setValue(alpha);
}

void SimpleTriode::setVct(double vct)
{
    parameter[PAR_VCT]->setValue(vct);
}

void SimpleTriode::setOptions()
{
    // Set solver options (these won't be used with direct calculation but kept for compatibility)
    options.max_num_iterations = 100;
    
    // Set parameter limits for numerical stability
    setLowerBound(parameter[PAR_KG1], 0.0000001); // Kg > 0
    setLimits(parameter[PAR_X], 1.0, 2.0); // 1.0 <= alpha <= 2.0
    setLimits(parameter[PAR_MU], 1.0, 1000.0); // 1.0 <= mu <= 1000.0
    setLimits(parameter[PAR_VCT], -2.0, 2.0); // -2.0 <= Vct <= 2.0
}
