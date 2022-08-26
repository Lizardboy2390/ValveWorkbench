#include "cohenhelietriode.h"

struct CohenHelieTriodeResidual {
    CohenHelieTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const kp, const T* const kvb, const T* const kvb2, const T* const vct, const T* const a, const T* const mu, T* residual) const {
        T e2t = log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg_ + vct[0]) / sqrt(kvb[0] + va_ * va_ + kvb2[0] * va_))));
        T ia = pow((va_ / kp[0]) * e2t, a[0]) / kg[0];
        residual[0] = ia_ - ia;
        return !(isnan(ia) || isinf(ia));
    }

private:
    const double va_;
    const double vg_;
    const double ia_;
};

CohenHelieTriode::CohenHelieTriode()
{
    parameter[PAR_KVB1] = new Parameter("Kvb2:", 30.0);
}

void CohenHelieTriode::addSample(double va, double ia, double vg1, double vg2)
{
    problem.AddResidualBlock(
        new AutoDiffCostFunction<CohenHelieTriodeResidual, 1, 1, 1, 1, 1, 1, 1, 1>(
            new CohenHelieTriodeResidual(va, vg1, ia)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_KP]->getPointer(),
        parameter[PAR_KVB]->getPointer(),
        parameter[PAR_KVB1]->getPointer(),
        parameter[PAR_VCT]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer());
}

double CohenHelieTriode::anodeCurrent(double va, double vg1, double vg2)
{
    return cohenHelieCurrent(va, vg1,
        parameter[PAR_KP]->getValue(),
        parameter[PAR_KVB]->getValue(),
        parameter[PAR_KVB1]->getValue(),
        parameter[PAR_VCT]->getValue(),
        parameter[PAR_X]->getValue(),
        parameter[PAR_MU]->getValue()) / parameter[PAR_KG1]->getValue();
}

void CohenHelieTriode::fromJson(QJsonObject source)
{
    KorenTriode::fromJson(source);

    if (source.contains("kvb2") && source["kvb2"].isDouble()) {
        parameter[PAR_KVB1]->setValue(source["kvb2"].toDouble());
    }
}

void CohenHelieTriode::toJson(QJsonObject &destination, double vg1Max, double vg2Max)
{
    QJsonObject model;
    model["kg"] = parameter[PAR_KG1]->getValue();
    model["mu"] = parameter[PAR_MU]->getValue();
    model["alpha"] = parameter[PAR_X]->getValue();
    model["vct"] = parameter[PAR_VCT]->getValue();
    model["kp"] = parameter[PAR_KP]->getValue();
    model["kvb"] = parameter[PAR_KVB]->getValue();
    model["kvb2"] = parameter[PAR_KVB1]->getValue();

    QJsonObject triode;
    triode["vg1Max"] = vg1Max;
    triode["improvedKoren"] = model;

    destination["triode"] = triode;
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
    return QString(" Cohen Helie");
}

void CohenHelieTriode::setOptions()
{
    KorenTriode::setOptions();

    setLimits(parameter[PAR_KVB1], 0.0, 1000.0); // 0.0 <= Kvb2 <= 1000.0
    setLimits(parameter[PAR_VCT], 0.0, 2.0); // 0.0 <= Vct <= 2.0
    options.linear_solver_type = ceres::CGNR;
    options.preconditioner_type = ceres::JACOBI;
}

double CohenHelieTriode::cohenHelieCurrent(double va, double vg, double kp, double kvb, double kvb2, double vct, double a, double mu)
{
    double x1 = std::sqrt(kvb + va * va + va * kvb2);
    double x2 = kp * (1 / mu + (vg + vct) / x1);
    double x3 = std::log(1.0 + std::exp(x2));
    double et = (va / kp) * x3;

    if (et < 0.0) {
        et = 0.0;
    }

    return pow(et, a);
}
