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

}

void CohenHelieTriode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    anodeProblem.AddResidualBlock(
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
    //options.use_inner_iterations = true;
    //options.use_nonmonotonic_steps = true;
    //options.trust_region_strategy_type = ceres::LEVENBERG_MARQUARDT;
    //options.trust_region_strategy_type = ceres::DOGLEG;
    //options.linear_solver_type = ceres::CGNR;
    options.linear_solver_type = ceres::DENSE_QR;
    //options.linear_solver_type = ceres::DENSE_NORMAL_CHOLESKY;
    //options.preconditioner_type = ceres::JACOBI;
    options.preconditioner_type = ceres::SUBSET;
}

double CohenHelieTriode::cohenHelieCurrent(double v, double vg, double kg1, double kp, double kvb, double kvb1, double vct, double x, double mu)
{
    qInfo("DEBUG: Kg1 = %.6f", kg1);
    return cohenHelieEpk(v, vg, kp, kvb, kvb1, vct, x, mu) / kg1;
}

double CohenHelieTriode::cohenHelieEpk(double v, double vg, double kp, double kvb, double kvb1, double vct, double x, double mu)
{
    // Debug parameter values
    qInfo("Cohen-Helie params: v=%.3f, vg=%.3f, kp=%.3f, kvb=%.3f, kvb1=%.3f, vct=%.3f, x=%.3f, mu=%.3f",
          v, vg, kp, kvb, kvb1, vct, x, mu);

    // Check for invalid inputs
    if (std::isnan(v) || std::isnan(vg) || std::isnan(kp) || std::isnan(kvb) || std::isnan(kvb1) ||
        std::isnan(vct) || std::isnan(x) || std::isnan(mu)) {
        qInfo("NaN detected in inputs - returning 0");
        return 0.0;
    }

    // Bounds check vg (grid voltage should be reasonable for vacuum tubes)
    if (vg < -10.0 || vg > 10.0) {
        qInfo("vg=%.3f out of bounds [-10, 10] - returning 0", vg);
        return 0.0;
    }

    // Prevent division by zero or negative values in sqrt
    double f = std::sqrt(std::max(0.0, kvb + v * kvb1 + v * v));
    if (f == 0.0) {
        qInfo("f is zero - returning 0");
        return 0.0;
    }

    double y = kp * (1.0 / mu + (vg + vct) / f);
    qInfo("Intermediate values: f=%.3f, y=%.3f", f, y);

    // Prevent overflow in exp
    if (y > 50.0) {
        qInfo("y=%.3f > 50, capping to 50", y);
        y = 50.0;
    }

    double ep = (v / kp) * std::log(1.0 + std::exp(y));
    qInfo("ep=%.3f", ep);

    // Prevent negative or zero values before pow
    if (ep <= 0.0) {
        qInfo("ep=%.3f <= 0 - returning 0", ep);
        return 0.0;
    }

    double result = std::pow(ep, x);
    qInfo("pow(ep, x) = %.3f", result);

    // Check for overflow
    if (std::isinf(result) || std::isnan(result) || result > 1e6) {
        qInfo("Overflow detected: result=%.3f - returning 0", result);
        return 0.0;
    }

    qInfo("Final result: %.3f", result);
    return result;
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
