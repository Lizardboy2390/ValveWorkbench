#include "simpletriode.h"

struct SimpleTriodeResidual {
    SimpleTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const vct, const T* const a, const T* const mu, T* residual) const {
        T e1t = va_ / mu[0] + vg_ + vct[0];
        if (e1t < 0.0) {
            e1t = mu[0] - mu[0];
        }
        T ia = pow(e1t, a[0]) / kg[0];
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
    parameter[PAR_KG1] = new Parameter("Kg:", 0.7);
    parameter[PAR_VCT] = new Parameter("Vct:", 0.1);
    parameter[PAR_X] = new Parameter("Alpha:", 1.5);
    parameter[PAR_MU] = new Parameter("Mu:", 100.0);
}

void SimpleTriode::addSample(double va, double ia, double vg1, double vg2)
{
    problem.AddResidualBlock(
        new AutoDiffCostFunction<SimpleTriodeResidual, 1, 1, 1, 1, 1>(
            new SimpleTriodeResidual(va, vg1, ia)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_VCT]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer());
}

double SimpleTriode::anodeCurrent(double va, double vg1, double vg2)
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
    if (source.contains("kg") && source["kg"].isDouble()) {
        parameter[PAR_KG1]->setValue(source["kg"].toDouble());
    }

    if (source.contains("mu") && source["mu"].isDouble()) {
        parameter[PAR_MU]->setValue(source["mu"].toDouble());
    }

    if (source.contains("alpha") && source["alpha"].isDouble()) {
        parameter[PAR_X]->setValue(source["alpha"].toDouble());
    }

    if (source.contains("vct") && source["vct"].isDouble()) {
        parameter[PAR_VCT]->setValue(source["vct"].toDouble());
    }
}

void SimpleTriode::toJson(QJsonObject &destination, double vg1Max, double vg2Max)
{
    QJsonObject model;
    model["kg"] = parameter[PAR_KG1]->getValue();
    model["mu"] = parameter[PAR_MU]->getValue();
    model["alpha"] = parameter[PAR_X]->getValue();
    model["vct"] = parameter[PAR_VCT]->getValue();

    QJsonObject triode;
    triode["vg1Max"] = vg1Max;
    triode["simple"] = model;

    destination["triode"] = triode;
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
