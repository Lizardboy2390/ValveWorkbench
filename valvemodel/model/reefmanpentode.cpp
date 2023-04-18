#include "reefmanpentode.h"

#include <cmath>

struct DerkPentodeResidual {
    DerkPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T g = 1.0 / (1.0 + beta[0] * va_);
        T ia = epk * ((1.0 / kg1[0] - 1.0 / kg2[0]) * (1.0 - g) + a[0] * va_ / kg1[0]);
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

struct DerkEPentodeResidual {
    DerkEPentodeResidual(double va, double vg1, double ia, double vg2, double ig2) : va_(va), vg1_(vg1), ia_(ia), vg2_(vg2), ig2_(ig2) {}

    template <typename T>
    bool operator()(const T* const kg1, const T* const kp, const T* const kvb, const T* const kvb1, const T* const vct, const T* const x, const T* const mu, const T* const kg2, const T* const a, const T* const alpha, const T* const beta, T* residual) const {
        T f = sqrt(kvb[0] + kvb1[0] * vg2_ + vg2_ * vg2_);
        T epk = pow(vg2_ * log(1.0 + exp(kp[0] * (1.0 / mu[0] + (vg1_ + vct[0]) / f))) / kp[0], x[0]);
        T g = exp(-pow(beta[0] * va_, 1.5));
        T ia = epk * ((1.0 / kg1[0] - 1.0 / kg2[0]) * (1.0 - g) + a[0] * va_ / kg1[0]);
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

double ReefmanPentode::anodeCurrent(double va, double vg1, double vg2)
{
    double epk = cohenHelieEpk(vg2, vg1);
    double k = 1.0 / parameter[PAR_KG1]->getValue() - 1.0 / parameter[PAR_KG2]->getValue();
    double shift = parameter[PAR_BETA]->getValue() * (1.0 - parameter[PAR_ALPHA]->getValue() * vg1);
    //double g = exp(-pow(shift * va, parameter[PAR_GAMMA]->getValue()));
    double g = 1.0 / (1.0 + pow(shift * va, parameter[PAR_GAMMA]->getValue()));
    double scale = 1.0 - g;
    double ia = epk * (k * scale + parameter[PAR_A]->getValue() * va / parameter[PAR_KG1]->getValue());

    return ia;
}

ReefmanPentode::ReefmanPentode(int newType) : modelType(newType)
{
    secondaryEmission = false;
}

void ReefmanPentode::addSample(double va, double ia, double vg1, double vg2, double ig2)
{
    if (modelType == DERK) {
        anodeProblem.AddResidualBlock(
            new AutoDiffCostFunction<DerkPentodeResidual, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(
                new DerkPentodeResidual(va, vg1, ia, vg2, ig2)),
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
            parameter[PAR_BETA]->getPointer());
    } else {
        anodeProblem.AddResidualBlock(
            new AutoDiffCostFunction<DerkEPentodeResidual, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1>(
                new DerkEPentodeResidual(va, vg1, ia, vg2, ig2)),
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
            parameter[PAR_BETA]->getPointer());
    }

}

void ReefmanPentode::fromJson(QJsonObject source)
{

}

void ReefmanPentode::toJson(QJsonObject &destination)
{

}

void ReefmanPentode::updateUI(QLabel *labels[], QLineEdit *values[])
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
    //updateParameter(labels[i], values[i], parameter[PAR_ALPHA]); i++;
    updateParameter(labels[i], values[i], parameter[PAR_BETA]); i++;
}

QString ReefmanPentode::getName()
{
    return "Cohen Helie Pentode";
}

int ReefmanPentode::getType()
{
    return REEFMAN_DERK_PENTODE;
}

void ReefmanPentode::updateProperties(QTableWidget *properties)
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
    //addProperty(properties, "alpha", QString("%1").arg(parameter[PAR_ALPHA]->getValue()));
    addProperty(properties, "beta", QString("%1").arg(parameter[PAR_BETA]->getValue()));
}

bool ReefmanPentode::withSecondaryEmission() const
{
    return secondaryEmission;
}

void ReefmanPentode::setSecondaryEmission(bool newSecondaryEmission)
{
    secondaryEmission = newSecondaryEmission;
}

int ReefmanPentode::getModelType() const
{
    return modelType;
}

void ReefmanPentode::setModelType(int newModelType)
{
    modelType = newModelType;
}

void ReefmanPentode::setOptions()
{
    anodeProblem.SetParameterBlockConstant(parameter[PAR_MU]->getPointer());
    //problem.SetParameterBlockConstant(parameter[PAR_X]->getPointer());
    anodeProblem.SetParameterBlockConstant(parameter[PAR_KP]->getPointer());
    anodeProblem.SetParameterBlockConstant(parameter[PAR_KG1]->getPointer());
    anodeProblem.SetParameterBlockConstant(parameter[PAR_KVB]->getPointer());
    anodeProblem.SetParameterBlockConstant(parameter[PAR_KVB1]->getPointer());
    anodeProblem.SetParameterBlockConstant(parameter[PAR_VCT]->getPointer());

    anodeProblem.SetParameterLowerBound(parameter[PAR_A]->getPointer(), 0, 0.0);
    anodeProblem.SetParameterLowerBound(parameter[PAR_ALPHA]->getPointer(), 0, 0.0);
    anodeProblem.SetParameterLowerBound(parameter[PAR_BETA]->getPointer(), 0, 0.0);

    options.max_num_iterations = 200;
    options.max_num_consecutive_invalid_steps = 20;
    options.use_inner_iterations = true;
    options.linear_solver_type = ceres::DENSE_QR;
    options.preconditioner_type = ceres::SUBSET;
}
