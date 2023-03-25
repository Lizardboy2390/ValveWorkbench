#include "cohenheliepentode.h"

#include <cmath>

struct UnifiedPentodeResidual {
    UnifiedPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, const T* const gamma, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T shift = beta[0] * (1.0 - alpha[0] * vg1_);
        T j = exp(-pow(shift * va_, gamma[0]));
        T scale = 1.0 - j;
        T ia = epk * ((1.0 / kg1[0] - 1.0 / kg2[0]) * scale + a[0] * va_ / kg1[0]);
        residual[0] = ia_ - ia;
        return !(isnan(ia) || isinf(ia));
    }

private:
    const double va_;
    const double vg1_;
    const double ia_;
    const double vg2_;
    const double ig2_;
};

double CohenHeliePentode::anodeCurrent(double va, double vg1, double vg2)
{
    double epk = cohenHelieEpk(vg2, vg1);
    double k = 1.0 / parameter[PAR_KG1]->getValue() - 1.0 / parameter[PAR_KG2]->getValue();
    double shift = parameter[PAR_BETA]->getValue() * (1.0 - parameter[PAR_ALPHA]->getValue() * vg1);
    double j = exp(-pow(shift * va, parameter[PAR_GAMMA]->getValue()));
    double scale = 1.0 - j;
    double ia = epk * (k * scale + parameter[PAR_A]->getValue() * va / parameter[PAR_KG1]->getValue());

    return ia;
}

CohenHeliePentode::CohenHeliePentode(int newType) : deviceType(newType)
{
    secondaryEmission = false;
}

void CohenHeliePentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    problem.AddResidualBlock(
        new AutoDiffCostFunction<UnifiedPentodeResidual, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(
            new UnifiedPentodeResidual(va, vg1, ia, vg2, ig2)),
        NULL,
        parameter[PAR_KG1]->getPointer(),
        parameter[PAR_KP]->getPointer(),
        parameter[PAR_KVB]->getPointer(),
        parameter[PAR_KVB1]->getPointer(),
        parameter[PAR_VCT]->getPointer(),
        parameter[PAR_X]->getPointer(),
        parameter[PAR_MU]->getPointer(),
        parameter[PAR_KG2]->getPointer(),
        parameter[PAR_A]->getPointer(),
        parameter[PAR_ALPHA]->getPointer(),
        parameter[PAR_BETA]->getPointer(),
        parameter[PAR_GAMMA]->getPointer());
}

void CohenHeliePentode::fromJson(QJsonObject source)
{

}

void CohenHeliePentode::toJson(QJsonObject &destination, double vg1Max, double vg2Max)
{

}

void CohenHeliePentode::updateUI(QLabel *labels[], QLineEdit *values[])
{
    int i = 0;

    updateParameter(labels[i], values[i], parameter[PAR_MU]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KG1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_X]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KP]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_KVB1]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_VCT]); i++;

    updateParameter(labels[i], values[i], parameter[PAR_KG2]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_A]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_GAMMA]); i++;
}

QString CohenHeliePentode::getName()
{
    return "Cohen Helie Pentode";
}

int CohenHeliePentode::getType()
{
    return COHEN_HELIE_PENTODE;
}

void CohenHeliePentode::updateProperties(QTableWidget *properties)
{
    clearProperties(properties);

    addProperty(properties, "Mu", QString("%1").arg(parameter[PAR_MU]->getValue()));
    addProperty(properties, "Kg1", QString("%1").arg(parameter[PAR_KG1]->getValue()));
    addProperty(properties, "X", QString("%1").arg(parameter[PAR_X]->getValue()));
    addProperty(properties, "Kp", QString("%1").arg(parameter[PAR_KP]->getValue()));
    addProperty(properties, "Kvb", QString("%1").arg(parameter[PAR_KVB]->getValue()));
    addProperty(properties, "Kvb1", QString("%1").arg(parameter[PAR_KVB1]->getValue()));
    addProperty(properties, "vct", QString("%1").arg(parameter[PAR_VCT]->getValue()));

    addProperty(properties, "Kg2", QString("%1").arg(parameter[PAR_KG2]->getValue()));
    addProperty(properties, "A", QString("%1").arg(parameter[PAR_A]->getValue()));
    addProperty(properties, "alpha", QString("%1").arg(parameter[PAR_ALPHA]->getValue()));
    addProperty(properties, "beta", QString("%1").arg(parameter[PAR_BETA]->getValue()));
    addProperty(properties, "gamma", QString("%1").arg(parameter[PAR_GAMMA]->getValue()));
}

int CohenHeliePentode::getDeviceType() const
{
    return deviceType;
}

void CohenHeliePentode::setDeviceType(int newType)
{
    deviceType = newType;
}

bool CohenHeliePentode::withSecondaryEmission() const
{
    return secondaryEmission;
}

void CohenHeliePentode::setSecondaryEmission(bool newSecondaryEmission)
{
    secondaryEmission = newSecondaryEmission;
}

void CohenHeliePentode::setOptions()
{
    problem.SetParameterBlockConstant(parameter[PAR_MU]->getPointer());
    //problem.SetParameterBlockConstant(parameter[PAR_X]->getPointer());
    problem.SetParameterBlockConstant(parameter[PAR_KP]->getPointer());
    problem.SetParameterBlockConstant(parameter[PAR_KG1]->getPointer());
    problem.SetParameterBlockConstant(parameter[PAR_KVB]->getPointer());
    problem.SetParameterBlockConstant(parameter[PAR_KVB1]->getPointer());
    problem.SetParameterBlockConstant(parameter[PAR_VCT]->getPointer());

    //problem.SetParameterBlockConstant(parameter[PAR_KG2]->getPointer());
    //problem.SetParameterBlockConstant(parameter[PAR_A]->getPointer());
    //problem.SetParameterBlockConstant(parameter[PAR_ALPHA]->getPointer());
    //problem.SetParameterBlockConstant(parameter[PAR_BETA]->getPointer());
    problem.SetParameterBlockConstant(parameter[PAR_GAMMA]->getPointer());

    problem.SetParameterLowerBound(parameter[PAR_A]->getPointer(), 0, 0.0);
    problem.SetParameterLowerBound(parameter[PAR_ALPHA]->getPointer(), 0, 0.0);
    problem.SetParameterLowerBound(parameter[PAR_BETA]->getPointer(), 0, 0.0);
    problem.SetParameterLowerBound(parameter[PAR_GAMMA]->getPointer(), 0, 0.5);
    problem.SetParameterUpperBound(parameter[PAR_GAMMA]->getPointer(), 0, 2.0);

    options.max_num_iterations = 200;
    options.max_num_consecutive_invalid_steps = 20;

    options.linear_solver_type = ceres::CGNR;
    options.preconditioner_type = ceres::JACOBI;
}
