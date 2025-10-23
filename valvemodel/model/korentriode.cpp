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
    anodeProblem.AddResidualBlock(
        new AutoDiffCostFunction<KorenTriodeResidual, 1, 1, 1, 1, 1, 1>(
            new KorenTriodeResidual(va, vg1, ia)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_KP]->getPointer(),
        parameter[PAR_KVB]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer());
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

    setLimits(parameter[PAR_KVB], 0.0, 10000.0); // 0 <= Kvb <= 10000.0
    options.linear_solver_type = ceres::CGNR;
    options.preconditioner_type = ceres::JACOBI;
}

double KorenTriode::korenCurrent(double va, double vg, double kp, double kvb, double a, double mu)
{
    // Check for invalid inputs
    if (std::isnan(va) || std::isnan(vg) || std::isnan(kp) || std::isnan(kvb) || std::isnan(a) || std::isnan(mu)) {
        return 0.0;
    }

    // Prevent negative values in sqrt
    double x1 = std::sqrt(std::max(0.0, kvb + va * va));
    if (x1 == 0.0) {
        return 0.0;
    }

    double x2 = kp * (1.0 / mu + vg / x1);

    // Prevent overflow in exp
    if (x2 > 50.0) {
        x2 = 50.0;
    }

    double x3 = std::log(1.0 + std::exp(x2));

    double et = (va / kp) * x3;

    // Prevent negative values before pow
    if (et <= 0.0) {
        return 0.0;
    }

    double result = std::pow(et, a);

    // Check for overflow
    if (std::isinf(result) || std::isnan(result) || result > 1e6) {
        return 0.0;
    }

    return result;
}
