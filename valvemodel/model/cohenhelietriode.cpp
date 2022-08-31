#include "cohenhelietriode.h"

struct CohenHelieTriodeResidual {
    CohenHelieTriodeResidual(double va, double vg, double ia) : va_(va), vg_(vg), ia_(ia) {}

    template <typename T>
    bool operator()(const T* const kg, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, T* residual) const {
        T epk = log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg_ + vct[0]) / sqrt(kvb[0] + va_ * va_ + kvb1[0] * va_))));
        T ia = pow((va_ / kp[0]) * epk, x[0]) / kg[0];
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
        parameter[PAR_KG1]->getValue(),
        parameter[PAR_KP]->getValue(),
        parameter[PAR_KVB]->getValue(),
        parameter[PAR_KVB1]->getValue(),
        parameter[PAR_VCT]->getValue(),
        parameter[PAR_X]->getValue(),
        parameter[PAR_MU]->getValue());
}

void CohenHelieTriode::fromJson(QJsonObject source)
{
    KorenTriode::fromJson(source);

    if (source.contains("kvb1") && source["kvb1"].isDouble()) {
        parameter[PAR_KVB1]->setValue(source["kvb1"].toDouble());
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
    model["kvb1"] = parameter[PAR_KVB1]->getValue();

    QJsonObject triode;
    triode["vg1Max"] = vg1Max;
    triode["cohenHelie"] = model;

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
    return QString("Cohen Helie");
}

void CohenHelieTriode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue()));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));
    addProperty(properties, "Kvb1", QString("%1").arg(parameter[PAR_KVB1]->getValue()));
    addProperty(properties, "vct", QString("%1").arg(parameter[PAR_VCT]->getValue()));
}

void CohenHelieTriode::setOptions()
{
    KorenTriode::setOptions();

    setLimits(parameter[PAR_KVB1], 0.0, 1000.0); // 0.0 <= Kvb2 <= 1000.0
    setLimits(parameter[PAR_VCT], 0.0, 2.0); // 0.0 <= Vct <= 2.0
    options.linear_solver_type = ceres::CGNR;
    options.preconditioner_type = ceres::JACOBI;
}

double CohenHelieTriode::cohenHelieCurrent(double v, double vg, double kg1, double kp, double kvb, double kvb1, double vct, double x, double mu)
{
    double f = std::sqrt(kvb + v * v + v * kvb1);
    double y = kp * (1 / mu + (vg + vct) / f);
    double ep = (v / kp) * std::log(1.0 + std::exp(y));

    return pow(ep, x) / kg1;
}
