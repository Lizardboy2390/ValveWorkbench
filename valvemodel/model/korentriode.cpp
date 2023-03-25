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
    problem.AddResidualBlock(
        new AutoDiffCostFunction<KorenTriodeResidual, 1, 1, 1, 1, 1, 1>(
            new KorenTriodeResidual(va, vg1, ia)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_KP]->getPointer(),
        parameter[PAR_KVB]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer());
}

double KorenTriode::anodeCurrent(double va, double vg1, double vg2)
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

void KorenTriode::toJson(QJsonObject &destination, double vg1Max, double vg2Max)
{
    QJsonObject model;
    model["kg"] = parameter[PAR_KG1]->getValue();
    model["mu"] = parameter[PAR_MU]->getValue();
    model["alpha"] = parameter[PAR_X]->getValue();
    model["vct"] = parameter[PAR_VCT]->getValue();
    model["kp"] = parameter[PAR_KP]->getValue();
    model["kvb"] = parameter[PAR_KVB]->getValue();

    QJsonObject triode;
    triode["vg1Max"] = vg1Max;
    triode["koren"] = model;

    destination["triode"] = triode;
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
    double x1 = std::sqrt(kvb + va * va);
    double x2 = kp * (1 / mu + vg / x1);
    double x3 = std::log(1.0 + std::exp(x2));
    double et = (va / kp) * x3;

    if (et < 0.0) {
        et = 0.0;
    }

    return pow(et, a);
}
