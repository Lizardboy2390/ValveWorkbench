#include "simpletriode.h"

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
        return !(isnan(ia) || isinf(ia));
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
    anodeProblem.AddResidualBlock(
        new AutoDiffCostFunction<SimpleTriodeResidual, 1, 1, 1, 1, 1>(
            new SimpleTriodeResidual(va, vg1, ia)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_VCT]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer());
}

double SimpleTriode::triodeAnodeCurrent(double va, double vg1)
{
    return anodeCurrent(va, vg1);
}

double SimpleTriode::anodeCurrent(double va, double vg1, double vg2, bool secondaryEmission)
{
    // Check for invalid inputs
    if (std::isnan(va) || std::isnan(vg1) || std::isnan(vg2)) {
        return 0.0;
    }

    double ia = 0.0;

    double e1t = va / parameter[PAR_MU]->getValue() + vg1 + parameter[PAR_VCT]->getValue();

    // Only calculate current if e1t is positive (tube conducting)
    if (e1t > 0.0) {
        double exponent = parameter[PAR_X]->getValue();
        double base = e1t;

        // Check for overflow before pow
        if (exponent > 0 && base > 0) {
            double log_base = std::log(base);
            if (log_base * exponent > 50.0) {
                // Would overflow, return safe value
                ia = 1e6; // Cap at reasonable maximum
            } else {
                ia = std::pow(base, exponent) / parameter[PAR_KG1]->getValue();
            }
        }
    }

    // Check for overflow
    if (std::isinf(ia) || std::isnan(ia) || ia > 1e6) {
        return 0.0;
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
    options.max_num_iterations = 100;
    options.minimizer_progress_to_stdout = true;

    setLowerBound(parameter[PAR_KG1], 0.0000001); // Kg > 0
    setLimits(parameter[PAR_X], 1.0, 2.0); // 1.0 <= alpha <= 2.0
    setLimits(parameter[PAR_MU], 1.0, 1000.0); // 1.0 <= mu <= 1000.0
    setLimits(parameter[PAR_VCT], -2.0, 2.0); // -2.0 <= Vct <= 2.0
}
