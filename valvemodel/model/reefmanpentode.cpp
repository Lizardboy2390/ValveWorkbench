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
    // Use the same g(Va) formulation as the Derk/DerkE residuals so that the
    // plotted anodeCurrent matches exactly what the Ceres solver is fitting.
    // For DERK:  g = 1 / (1 + beta * Va)
    // For DERK_E: g = exp( - (beta * Va)^(3/2) )

    const double epk = cohenHelieEpk(vg2, vg1);
    const double kg1 = parameter[PAR_KG1]->getValue();
    const double kg2 = parameter[PAR_KG2]->getValue();
    const double a   = parameter[PAR_A]->getValue();
    const double beta = parameter[PAR_BETA]->getValue();

    if (epk <= 0.0 || kg1 <= 0.0 || kg2 <= 0.0) {
        return 0.0;
    }

    const double k = 1.0 / kg1 - 1.0 / kg2;

    double g;
    if (modelType == DERK_E) {
        // DerkE / DEPIa-style knee
        g = std::exp(-std::pow(beta * va, 1.5));
    } else {
        // Original Derk formulation
        g = 1.0 / (1.0 + beta * va);
    }

    const double scale = 1.0 - g;
    const double ia = epk * (k * scale + a * va / kg1);

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
    // Load common Cohen-Helie triode-style parameters first (mu, kg1, x, kp, kvb, kvb1, vct)
    CohenHelieTriode::fromJson(source);

    // Optional modelType hint allows JSON to distinguish between Derk and
    // DerkE variants when re-importing a saved model. Older files without
    // this field default to the original Derk formulation.
    const QString modelTypeStr = source.value("modelType").toString().trimmed().toUpper();
    if (modelTypeStr == QLatin1String("REEFMAN_DERK_E_PENTODE")) {
        modelType = DERK_E;
    } else {
        modelType = DERK;
    }

    // Reefman-specific pentode parameters (minimal set used in anodeCurrent/addSample)
    if (source.contains("kg2") && source["kg2"].isDouble()) {
        parameter[PAR_KG2]->setValue(source["kg2"].toDouble());
    }

    if (source.contains("a") && source["a"].isDouble()) {
        parameter[PAR_A]->setValue(source["a"].toDouble());
    }

    if (source.contains("alpha") && source["alpha"].isDouble()) {
        parameter[PAR_ALPHA]->setValue(source["alpha"].toDouble());
    }

    if (source.contains("beta") && source["beta"].isDouble()) {
        parameter[PAR_BETA]->setValue(source["beta"].toDouble());
    }
}

void ReefmanPentode::toJson(QJsonObject &destination)
{
    // Serialize the shared Cohen-Helie base parameters first
    CohenHelieTriode::toJson(destination);

    // Serialize the minimal Reefman pentode parameter set
    destination["kg2"]   = parameter[PAR_KG2]->getValue();
    destination["a"]     = parameter[PAR_A]->getValue();
    destination["alpha"] = parameter[PAR_ALPHA]->getValue();
    destination["beta"]  = parameter[PAR_BETA]->getValue();

    destination["device"] = "pentode";
    destination["type"]   = "reefman";

    // Preserve the specific Reefman variant so re-import can recreate the
    // correct model type without relying on external metadata.
    if (modelType == DERK_E) {
        destination["modelType"] = "REEFMAN_DERK_E_PENTODE";
    } else {
        destination["modelType"] = "REEFMAN_DERK_PENTODE";
    }
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
    // Expose the concrete Reefman variant in the project tree so it is
    // obvious whether the Derk or DerkE (DEPIa-style) formulation is used.
    if (modelType == DERK_E) {
        return "Reefman Pentode (DerkE)";
    }
    return "Reefman Pentode (Derk)";
}

int ReefmanPentode::getType()
{
    // Map internal variant to the public eModelType so downstream code
    // (preferences, plotting, bounds) can distinguish them when needed.
    return (modelType == DERK_E) ? REEFMAN_DERK_E_PENTODE : REEFMAN_DERK_PENTODE;
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
    // Keep A clearly non-zero so the Va term contributes a real tail slope
    // instead of collapsing to ~0 and making the curve appear vertical.
    anodeProblem.SetParameterLowerBound(parameter[PAR_A]->getPointer(), 0, 0.005);
    anodeProblem.SetParameterLowerBound(parameter[PAR_ALPHA]->getPointer(), 0, 0.0);
    // Constrain beta so the knee cannot be extremely sharp, but allow more
    // range so Reefman can match earlier knees when required.
    anodeProblem.SetParameterLowerBound(parameter[PAR_BETA]->getPointer(), 0, 0.02);
    anodeProblem.SetParameterUpperBound(parameter[PAR_BETA]->getPointer(), 0, 0.30);

    options.max_num_iterations = 200;
    options.max_num_consecutive_invalid_steps = 20;
    options.use_inner_iterations = true;
    options.linear_solver_type = ceres::DENSE_QR;
    options.preconditioner_type = ceres::SUBSET;
}
